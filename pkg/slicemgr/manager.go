package slicemgr

import (
	"context"
	"fmt"
	"strconv"
	"time"

	rsmapi "github.com/onosproject/onos-api/go/onos/rsm"
	topoapi "github.com/onosproject/onos-api/go/onos/topo"
	e2sm_rsm "github.com/onosproject/onos-e2-sm/servicemodels/e2sm_rsm/v1/e2sm-rsm-ies"
	"github.com/onosproject/onos-lib-go/pkg/logging"
	"github.com/onosproject/onos-rsm/pkg/northbound"    // TODO:
	"github.com/onosproject/onos-rsm/pkg/southbound/e2" // TODO:
)

var log = logging.GetLogger()

func NewManager(opts ...Option) Manager { // start slicing manager with configs applyied in options.go
	log.Info("Init IQoS-xApp Slicing Manager")
	options := Options{}

	for _, opt := range opts {
		opt.apply(&options)
	}

	return Manager{ // return a new instance of Manager configured with defined options
		rsmMsgCh:              options.Chans.RsmMsgCh, // Included configs of communnications channels to control mensages related with create, update and delete slices
		ctrlReqChsSliceCreate: options.Chans.CtrlReqChsSliceCreate,
		//ctrlReqChsSliceUpdate: options.Chans.CtrlReqChsSliceUpdate,
		//ctrlReqChsSliceDelete: options.Chans.CtrlReqChsSliceDelete,
		//ctrlReqChsUeAssociate: options.Chans.CtrlReqChsUeAssociate,//control request channels to associate UEs in a specific slice
		rnibClient:     options.App.RnibClient, // clients RNIB and UENIB
		uenibClient:    options.App.UenibClient,
		ctrlMsgHandler: e2.NewControlMessageHandler(), // manipulator of control mensages
		ackTimer:       options.App.AckTimer,          // timer
	}
}

func (m *Manager) Run(ctx context.Context) { // cria um dispachador em uma go routine separada paranão bloquear mensagens de entradas simultaneas
	go m.DispatchNbiMsg(ctx)
}

func (m *Manager) DispatchNbiMsg(ctx context.Context) {
	log.Info("Run nbi msg dispatcher")
	for msg := range m.rsmMsgCh { // recebe em loop mensagens provindas do rsmMsgCh
		log.Debugf("Received message from NBI: %v", msg)
		var ack northbound.Ack
		var err error
		switch msg.Message.(type) { // processa a msg com base em seu tipo, chamando a função de manipulação respectiva
		case *rsmapi.CreateSliceRequest:
			err = m.handleNbiCreateSliceRequest(ctx, msg.Message.(*rsmapi.CreateSliceRequest), msg.NodeID)
		//case *rsmapi.UpdateSliceRequest:
		//	err = m.handleNbiUpdateSliceRequest(ctx, msg.Message.(*rsmapi.UpdateSliceRequest), msg.NodeID)
		//case *rsmapi.DeleteSliceRequest:
		//	err = m.handleNbiDeleteSliceRequest(ctx, msg.Message.(*rsmapi.DeleteSliceRequest), msg.NodeID)
		//case *rsmapi.SetUeSliceAssociationRequest:
		//	err = m.handleNbiSetUeSliceAssociationRequest(ctx, msg.Message.(*rsmapi.SetUeSliceAssociationRequest), msg.NodeID)
		default:
			err = fmt.Errorf("unknown msg type: %v", msg)
		}
		if err != nil { // caso der erro
			ack = northbound.Ack{
				Success: false,
				Reason:  err.Error(),
			}
		} else { // caso operação der certo
			ack = northbound.Ack{
				Success: true,
			}
		} // fornecendo um mecanismo de feedback claro para as operações iniciadas pelas mensagens da interface norte.
		msg.AckCh <- ack // indicando o sucesso ou falha da operação.
	}
}

func (m *Manager) handleNbiCreateSliceRequest(ctx context.Context, req *rsmapi.CreateSliceRequest, nodeID topoapi.ID) error {
	log.Infof("Called Create Slice: %v", req) //  função de criar fatia foi chamada
	sliceID, err := strconv.Atoi(req.SliceId) // Converte o ID da fatia e o peso de strings para inteiros
	if err != nil {
		return fmt.Errorf("failed to convert slice id to int - %v", err.Error())
	}
	weightInt, err := strconv.Atoi(req.Weight)
	if err != nil {
		return fmt.Errorf("failed to convert weight to int - %v", err.Error()) // falhas na conversão
	}
	weight := int32(weightInt)

	cmdType := e2sm_rsm.E2SmRsmCommand_E2_SM_RSM_COMMAND_SLICE_CREATE
	var sliceSchedulerType e2sm_rsm.SchedulerType // atribuição de valores para o tipo de agendador
	switch req.SchedulerType {
	case rsmapi.SchedulerType_SCHEDULER_TYPE_ROUND_ROBIN:
		sliceSchedulerType = e2sm_rsm.SchedulerType_SCHEDULER_TYPE_ROUND_ROBIN
	case rsmapi.SchedulerType_SCHEDULER_TYPE_PROPORTIONALLY_FAIR:
		sliceSchedulerType = e2sm_rsm.SchedulerType_SCHEDULER_TYPE_PROPORTIONALLY_FAIR
	case rsmapi.SchedulerType_SCHEDULER_TYPE_QOS_BASED:
		sliceSchedulerType = e2sm_rsm.SchedulerType_SCHEDULER_TYPE_QOS_BASED
	default:
		sliceSchedulerType = e2sm_rsm.SchedulerType_SCHEDULER_TYPE_ROUND_ROBIN
	}

	var sliceType e2sm_rsm.SliceType// atribuição de valores para o tipo de slice
	switch req.SliceType {
	case rsmapi.SliceType_SLICE_TYPE_DL_SLICE:
		sliceType = e2sm_rsm.SliceType_SLICE_TYPE_DL_SLICE
	case rsmapi.SliceType_SLICE_TYPE_UL_SLICE:
		sliceType = e2sm_rsm.SliceType_SLICE_TYPE_UL_SLICE
	default:
		sliceType = e2sm_rsm.SliceType_SLICE_TYPE_DL_SLICE
	}

	sliceConfig := &e2sm_rsm.SliceConfig{ //criação do objeto com definição do ID e parâmetros de configuração
		SliceId: &e2sm_rsm.SliceId{
			Value: int64(sliceID),
		},
		SliceConfigParameters: &e2sm_rsm.SliceParameters{
			SchedulerType: sliceSchedulerType,
			Weight:        &weight,
		},
		SliceType: sliceType,
	}
	ctrlMsg, err := m.ctrlMsgHandler.CreateControlRequest(cmdType, sliceConfig, nil) //  manipulador de mensagens de controle para criar uma mensagem de controle E2 
	if err != nil {																	 //  destinada a executar a criação da fatia com a configuração especificada
		return fmt.Errorf("failed to create the control message - %v", err.Error())
	}

	hasSliceItem := m.rnibClient.HasRsmSliceItemAspect(ctx, topoapi.ID(req.E2NodeId), req.SliceId, req.GetSliceType()) // se a fatia já existe no RNIB para evitar duplicatas

	if hasSliceItem {
		return fmt.Errorf("slice ID %v already exists", sliceID)
	}

	// send control message
	ackCh := make(chan e2.Ack) // uma mensagem de controle para ser enviada ao controlador E2, criando um canal de reconhecimento
	msg := &e2.CtrlMsg{
		CtrlMsg: ctrlMsg,
		AckCh:   ackCh,
	}
	go func() {
		m.ctrlReqChsSliceCreate[string(nodeID)] <- msg // go routine que envia a mensagem de controle para o canal específico do ID do nó
	}()

	// ackTimer -1 is for uenib/topo debugging and integration test
	if m.ackTimer != -1 { // implementa a lógica de espera por um reconhecimento (ACK) da mensagem de controle enviada
		var ack e2.Ack
		select {
		case <-time.After(time.Duration(m.ackTimer) * time.Second):
			return fmt.Errorf("timeout happens: E2 SBI could not send ACK until timer expired")
		case ack = <-ackCh:
		}

		if !ack.Success {
			return fmt.Errorf("%v", ack.Reason)
		}
	}

	value := &topoapi.RSMSlicingItem{ // se a resposta for bem sucedida, continua o registro da nova fatia no RNIB
		ID:        req.SliceId, // cria-se um objeto RSMSlicingItem contendo os detalhes da fatia
		SliceDesc: "Slice created by onos-RSM xAPP",
		SliceParameters: &topoapi.RSMSliceParameters{
			SchedulerType: topoapi.RSMSchedulerType(req.SchedulerType),
			Weight:        weight,
		},
		SliceType: topoapi.RSMSliceType(req.SliceType),
		UeIdList:  make([]*topoapi.UeIdentity, 0),
	}

	err = m.rnibClient.AddRsmSliceItemAspect(ctx, topoapi.ID(req.E2NodeId), value) // Se o registro falhar por algum motivo
																				   // a função retorna um erro indicando a falha no registro da fatia no RNIB
	if err != nil {
		return fmt.Errorf("failed to create slice information to onos-topo although control message was sent: %v", err)
	}

	return nil //indica que a solicitação foi criada sem erros
}

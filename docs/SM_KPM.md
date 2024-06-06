# Service Model KPM_V3_00

## Supported Report Style Types
- **RIC Style Type 1**:  E2 Node Measurement. Used to carry measurement report from a target E2 Node. More in details,
                    it contains measurement types that Near-RT RIC is requesting to subscribe followed by a list 
                    of subcounters to be measured for each measurement type, and a granularity period
                    indicating collection interval of those measurements. (not implemented)


- **RIC Style Type 2**: Used to carry measurement report for a single UE of interest from a target E2 Node. (not implemented)


- **RIC Style Type 3**: Used to carry UE-level measurement report for a group of UEs per measurement type matching subscribed conditions from a target E2 Node. (:x:)


- **RIC Style Type 4**: Used to carry measurement report for a group of UEs across a set of measurement types satisfying common subscribed conditions from a target E2 Node. (:white_check_mark:)


- **RIC Style Type 5**: Used to carry measurement report for multiple UE of interest from a target E2 Node (not implemented).

## Supported measurements

The document 28_552_kpm_meas.txt contains the list of all measurements defined in 3GPP TS 28.552.

### The following measurements are supported in OAI-RAN:

- "DRB.PdcpSduVolumeDL"
- "DRB.PdcpSduVolumeUL"
- "DRB.RlcSduDelayDl"
- "DRB.UEThpDl"
- "DRB.UEThpUl"
- "RRU.PrbTotDl"
- "RRU.PrbTotUl"

### The following measurements are supported in SRS-RAN:

- "DRB.UEThpDl" - DL throughput
- "DRB.UEThpUl" - UL throughput
- "DRB.RlcPacketDropRateDl" - UL packet success rate
- "DRB.PacketSuccessRateUlgNBUu" - RLC DL packet drop rate
- "DRB.RlcSduTransmittedVolumeDL" - RLC DL transmitted SDU volume **(not found in spec)**
- "DRB.RlcSduTransmittedVolumeUL" - RLC UL transmitted SDU volume **(not found in spec)**
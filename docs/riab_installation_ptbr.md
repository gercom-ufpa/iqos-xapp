### Instalação SDRan-in-a-Box (RiaB)

Precisamos, inicialmente, com o comando `git clone` criar uma cópia do repositório do projeto SDRan-in-a-Box para o ambiente local. Em seguida, alteramos o diretório de trabalho para o diretório recém-clonado `sdran-in-a-box` (isso é necessário para que os próximos comando `git` sejam executados no contexto do repositório). Utilizamos o comando `git checkout`, alterando a branch atual para a branch 'master' (embora possa ser utilizadas outras branches, recomenda-se pela documentação utilizar a master). 

```shell
git clone https://github.com/onosproject/sdran-in-a-box
cd sdran-in-a-box 
git checkout master
```

No repositório, existem uma série de arquivos de configuração no formato `yaml`, por exemplo, o `sdran-in-a-box-values-master-stable.yaml` (o nome do arquivo pode variar dependendo da branch e da versão utilizada). Esses arquivos possuem algumas configurações referentes as UEs, CUs e DUs que podem ser alteradas (caso haja necessidade), por enquanto, para fins de primeira instalação podemos deixar essas configurações como padrão.

Um passo obrigatório na instalação é configurar alguns parâmetros de IP do arquivo `MakefileVar.mk` que contém instruções sobre como compilar e construir o RiaB. Precisamos alterar os parâmetros `OAI_MACHINE_IP`, `OMEC_MACHINE_IP` e `RIC_MACHINE_IP` para um IP apropriado (no caso, o IP da máquina que estamos utilizando). É nesse arquivo também que podemos verificar configurações de versionamento, como a versão do Helm e a versão do Kubernetes utilizadas.

Feita as alterações nos IPs, podemos construir o projeto com o comando (certifique-se que você tenha o pacote `make` instalado no seu sistema operacional):

```shell
make riab OPT=oai VER=stable
```

Esse comando instancia um cluster kubernetes, adiciona os charts e realiza a instalação do RIC e suas interfaces padrão, podemos utilizar o `OPT` para outros modos de instalação, nesse caso, instalamos também a OpenAirInterface para realizar a simulação das UEs, e ainda utilizar o `VER` para escolher a versão do projeto desejada. 


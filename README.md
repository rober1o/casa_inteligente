# CASA INTELIGENTE

O projeto visa desenvolver um sistema de automação residencial com controle remoto de dispositivos como luzes, alarmes e portas. A interface será acessada via servidor web, permitindo o gerenciamento fácil e eficiente dos dispositivos.

## Componentes Utilizados


1. **Botão Pushbutton**
2. **Display OLED 1306**
3. **Buzzer**
4. **Matriz de LED 5x5 WS2812** 
5. **Led RGB**
6. Modulo WIFI (CYW4)

## Funcionalidade

Este sistema de automação residencial permite ao usuário, por meio de uma interface web, realizar as seguintes operações:

Ligar/Desligar a Luz 1

Ligar/Desligar a Luz 2

Abrir/Fechar a Porta

Ativar/Desativar o Alarme

Ativar/Desativar o Modo Viagem

Monitorar a Temperatura Ambiente

Ao iniciar o sistema pela primeira vez, é necessário configurar a rede Wi-Fi (SSID e senha) para que o dispositivo se conecte à internet. Após a conexão bem-sucedida, o endereço IP será exibido via UART, permitindo o acesso à interface web por esse endereço.

### Como Usar

#### Usando a BitDogLab

- Clone este repositório: git clone https://github.com/rober1o/casa_inteligente.git;
- Usando a extensão Raspberry Pi Pico importar o projeto;
- Ajuste a rede wifi e senha 
- Compilar o projeto;
- Plugar a BitDogLab usando um cabo apropriado

## Demonstração
<!-- TODO: adicionar link do vídeo -->
Vídeo demonstrando as funcionalidades da solução implementada: [Demonstração](https://youtu.be/ZQJ13r0Qnfk)

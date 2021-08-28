# Persianas-Autonomas
Código utilizado para controle das persianas, desenvolvido para o *Trabalho de Conclusão do Curso de Sistemas de Informação*

Escopo do trabalho: controlar a abertura de persianase a luz artificial do ambiente, sem a interação constante do usuário.

A proposta: implementar um módulo controlador que recebe dados deum sensor de luminosidade e se comunica através de troca de mensagens com osmartphone do usuário a fim de receber o parâmetro da luminosidade desejada, apartir desse parâmetro, a placa controladora ajusta a abertura da persiana e senecessário a intensidade dos LEDs.

Dispositivos, sensores e atuadores utilizados:

* Placa controladora - ESP8266 NodeMCU Lolin V3
* Motor micro servo - SG90 TowerPro
* 1 x Fotoresistor - LDR
* 1 x Resistor 4.7 k\(\Omega\)
* 1 x NeoPixel Ring - 16x 5050 RGB LED


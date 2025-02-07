## 🐶 Sistema de Recompensa Automática para Pets  

🚀 **Descrição**  
Este projeto foi desenvolvido como parte da conclusão do curso **EmbarcaTech** e tem como objetivo simular um sistema automatizado de recompensa para treinar cães a utilizarem uma plataforma de xixi. A proposta consiste em detectar o peso do animal na plataforma e, após **5 segundos**, liberar uma recompensa acompanhada de um sinal sonoro.  

Na simulação atual, utilizei o **botão A da BitDogLab** para representar a ativação do sensor de peso, demonstrando a viabilidade do sistema.  

## 🔹 Benefícios
- Permite que tutores incentivem seus pets a fazerem xixi no local correto, por meio de um estímulo positivo (petisco).  
- Resolve um dos principais desafios do adestramento, garantindo que a recompensa ocorra **no momento exato do comportamento desejado**, mesmo quando o tutor estiver ausente.  
- Facilita a **educação do pet** e contribui para a **manutenção da higiene do ambiente**, tornando o processo mais eficiente e automatizado.  

Essa automação pode ser integrada a um protótipo real com **sensor de peso e dispenser automático**, oferecendo uma solução prática e inovadora para o adestramento de cães. 🐶🎯  


🔗 **Demonstração do projeto:** [YouTube - Demonstração](https://www.youtube.com/shorts/W83RpyJlnjA)  

---

## 📌 **Principais Funcionalidades**  
✅ Liberação automática de petiscos baseada em um **sensor de peso**  
✅ Feedback visual e sonoro com **LEDs e Buzzer**  
✅ Exibição de mensagens no **Display OLED**  
✅ Simulação do projeto utilizando **BitDogLab**  

---

## 🛠️ **Tecnologias e Componentes Utilizados**  
- **Microcontrolador**: RP2040 (Raspberry Pi Pico W)  
- **Sensores e Atuadores**:  
  - **Sensor de Peso** (simulado pelo botão A)  
  - **Matriz de LEDs WS2818B**  
  - **Buzzer**  
  - **Display OLED SSD1306**  
- **Linguagem de Programação**: C  
- **Bibliotecas Utilizadas**: `pico/stdlib.h`, `hardware/i2c.h`, `ssd1306.h`, `hardware/pwm.h`, `ws2818b.pio.h`  

---

## 📌 Futuras Melhorias  
- Implementação de **sensor de peso real** para substituir a simulação com o botão.  
- Desenvolvimento de um **dispenser automatizado** para liberação do petisco.  
- Criação de um **aplicativo ou interface web** para monitoramento remoto do uso da plataforma.  



//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                  Maquina de cortar brogadeiros
//
//   build 07/04/2023 by Raul
//  v1.0 REV. 100921
//   programmer CCS 5.015
//   debugger/programmer PICKit 3.10
//   microcontroler PIC 16f877
//   IDE CCS 5.015
//   schematic: Labcenter Proteus 8.9
//       @site:www.autorobotica.sp@gmail.com
//       @email: raulstar3@gmail.com�,
//       www.linkedin.com/in/raulstar/, Instagram: @raulstar3,github.com/raulstar
//    External dependencies:

//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <16F877.h> // Inclui o arquivo de cabe�alho espec�fico para o microcontrolador PIC16F877.

#FUSES WDT        // No Watch Dog Timer - Desativa o Watchdog Timer.
#FUSES PUT        // Power Up Timer - Ativa o Power-Up Timer.
#FUSES NOBROWNOUT // No brownout reset - Desativa o reset por brownout.
#FUSES NOLVP      // No low voltage prgming, B3(PIC16) or B5(PIC18) used for I/O - Desativa a programa��o de baixa tens�o e define o pino B3 (PIC16) ou B5 (PIC18) para I/O.

#DEVICE ADC = 8 // Configura a precis�o do conversor anal�gico-digital (ADC) para 8 bits.

#use delay(crystal = 4000000) // Configura a fun��o de delay para usar um oscilador de cristal de 4 MHz como fonte de clock.

#include <lcd.c> // Inclui o arquivo "lcd.c" que cont�m fun��es para o display LCD.

#use fast_io(A) // Habilita opera��es r�pidas de I/O para o porto A.

// Macros
#define DELAY 1000                         // Define a macro "DELAY" com o valor 1000.
#define LED PIN_B5                         // Define a macro "LED" como o pino B5.
#define Limit_superior input(PIN_A1)       // Define a macro "Limit_superior" como a leitura do pino A1.
#define Limit_inferior input(PIN_A2)       // Define a macro "Limit_inferior" como a leitura do pino A2.
#define liga input(PIN_E0)                 // Define a macro "liga" como a leitura do pino E0.
#define manual input(PIN_C1)               // Define a macro "manual" como a leitura do pino C1.
#define sobe input(PIN_A4)                 // Define a macro "sobe" como a leitura do pino A4.
#define desce input(PIN_A5)                // Define a macro "desce" como a leitura do pino A5.
#define habilita input(PIN_A3)             // Define a macro "habilita" como a leitura do pino A3.
#define conte input(PIN_E1)                // Define a macro "conte" como a leitura do pino E1.
#define menu input(PIN_C0)                 // Define a macro "menu" como a leitura do pino C0.
#define inverteMotor output_high(PIN_E2)   // Define a macro "inverteMotor" como a sa�da em n�vel alto (HIGH) para o pino E2.
#define desinverteMotor output_low(PIN_E2) // Define a macro "desinverteMotor" como a sa�da em n�vel baixo (LOW) para o pino E2.

// Variaveis
int16 acumulador, frequencia, contador; // Declara��o de vari�veis: acumulador, frequencia e contador do tipo int16.

signed int16 error_meas, kp = 30, ki = 0, kd = 30, proportional, integral, derivative, PID; // Declara��o de vari�veis: error_meas, kp, ki, kd, proportional, integral, derivative e PID do tipo int16, algumas com valores iniciais.

// constantes
int setpoit = 5, duty = 0, pwm_period = 168; // Valor do per�odo do PWM em us para 1.515 kHz;

signed int potencia = 0, measure = 50, lastMeasure = 80; // Declara��o de vari�veis: potencia, measure e lastMeasure do tipo int, com valores iniciais.

int funcao = 0; // Declara��o de vari�vel: funcao do tipo int, com valor inicial 0.

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fun��es
int16 frequecimetro(int16 acumulador);
void pid_control();
int leitura(void);
float pwm(signed int potencia);
int media(long frequencia);
void comtroleMotor(int duty, setpoit);
void contagem(void);
int16 lerMemoria(void);
void escreveMemoria(int16 variavel);
void modo(int funcao);
void atualizaDisplay(void);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Interrup��es
#INT_EXT
void EXT_isr(void)
{
  static int1 borda = 0;                  // Vari�vel est�tica para controle de borda
  frequencia = frequecimetro(acumulador); // Atualiza a vari�vel frequencia com base no valor do acumulador

  if (!borda) // Verifica se a borda � igual a 0
  {
    borda = 1;            // Define a borda como 1
    ext_int_edge(H_TO_L); // Configura a interrup��o externa para borda de descida
  }
  else
  {
    borda = 0;            // Define a borda como 0
    ext_int_edge(L_TO_H); // Configura a interrup��o externa para borda de subida
  }
  acumulador = 0; // Reinicia o acumulador
}

#INT_TIMER0
void TIMER0_isr(void)
{
  if (acumulador > 900) // Verifica se o valor do acumulador � maior que 900
    frequencia = 0;     // Define a frequ�ncia como 0
  acumulador++;         // Incrementa o valor do acumulador
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
void main()
{
   contador = lerMemoria(); // L� o valor do contador da mem�ria~
   // Configura��o
  delay_ms(10);     // Pequeno atraso de 10ms
  lcd_init();       // Inicializa o display LCD
  lcd_gotoxy(1, 1); // Posiciona o cursor na primeira linha, primeira coluna

  setup_timer_0(RTCC_INTERNAL | RTCC_DIV_1 | RTCC_DIV_4); // Configura o Timer0 com clock interno, divisor 1:4
  setup_ccp1(CCP_PWM);                                    // Configura o CCP1 para modo PWM
  setup_timer_2(T2_DIV_BY_1, 124, 1);                     // Configura o Timer2 com prescaler 1:1, per�odo de 1ms
  set_pwm1_duty(0);                                       // Define o duty cycle inicial do PWM como 0

  setup_adc_ports(AN0);          // Configura o pino AN0 como entrada anal�gica
  setup_adc(ADC_CLOCK_DIV_2);    // Configura o clock do ADC com divisor 2
  enable_interrupts(INT_EXT);    // Habilita a interrup��o externa
  ext_int_edge(L_TO_H);          // Configura a interrup��o externa para borda de subida
  enable_interrupts(INT_TIMER1); // Habilita a interrup��o do Timer1
  enable_interrupts(INT_TIMER0); // Habilita a interrup��o do Timer0
  enable_interrupts(GLOBAL);     // Habilita as interrup��es globais
  setup_wdt(WDT_2304MS);         // Configura o Watchdog Timer com um tempo de estouro de 2304ms

  printf(lcd_putc, "e0\r\n");                               // Imprime uma mensagem no display LCD
  delay_ms(50);                                             // Pequeno atraso de 50ms
  printf(lcd_putc, "\fCorta Brigadeiros\n           v1.0"); // Imprime uma mensagem no display LCD
  delay_ms(500);

 
                           //////////////////////////////////////////////////////////////////////////////////////////////////////////////
                           //
  while (true)
  {
    restart_wdt();                // Reinicia o Watchdog Timer
    setpoit = leitura();          // L� o valor do setpoint
    if (!setpoit)                 // Verifica se o setpoint � igual a 0
      duty = 0, potencia = 0;     // Define o duty cycle e a pot�ncia como 0
    pid_control();                // Chama a fun��o de controle PID
    duty = pwm(potencia);         // Calcula o valor do duty cycle do PWM com base na pot�ncia
    contagem();        // Realiza a contagem
    if (menu)                     // Verifica se o bot�o de menu est� pressionado
      contador = 0;               // Define o contador como 0
      atualizaDisplay();
    //modo(funcao);                 // Executa o modo correspondente � fun��o
    comtroleMotor(duty, setpoit); // Controla o motor com base no duty cycle e no setpoint
    delay_ms(10);                 // Pequeno atraso de 10ms
    escreveMemoria(contador);     // Escreve o valor do contador na mem�ria
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
int16 frequecimetro(int16 cont)
{
  frequencia = ((float)10000 / (cont * 5.12)); // Calcula a frequ�ncia atual do PWM
  float temp = (float)5 * frequencia;
  frequencia = (int)temp;
  return frequencia;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
void pid_control()
{
  int potenciaMinima = 10;                    // Define a pot�ncia m�nima como 10
  measure = frequencia;                       // Atribui o valor da frequ�ncia � vari�vel measure
  error_meas = setpoit - measure;             // Calcula o erro de medi��o
  proportional = error_meas * kp;             // Calcula a parte proporcional do controle PID
  integral += error_meas * ki;                // Calcula a parte integral do controle PID
  derivative = (lastMeasure - measure) * kd;  // Calcula a parte derivativa do controle PID
  lastMeasure = measure;                      // Atualiza o valor da �ltima medida
  PID = proportional + integral + derivative; // Calcula o valor do controle PID
  // potencia = PID + setpoit;

  // Limita a pot�ncia dentro de um intervalo m�nimo
  if (potencia < (potenciaMinima))
    potencia = potenciaMinima;

  // Limita a pot�ncia para n�o ultrapassar o valor de PID + setpoit
  if (potencia > (PID + setpoit))
    potencia = potencia - 1;

  // Aumenta a pot�ncia gradualmente at� atingir PID + setpoit
  if (potencia < (PID + setpoit))
    potencia++;

  // Limita a pot�ncia m�xima em 100
  if (potencia >= 100)
    potencia = 100;

  // Limita a pot�ncia m�nima em 0
  if (potencia <= 0)
    potencia = 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
int leitura(void)
{
  int value;
  float temp;
  set_adc_channel(0);                // Configura o canal do ADC como 0
  value = read_adc();                // L� o valor do ADC
  temp = ((float)100 / 255) * value; // Calcula o valor da leitura como uma porcentagem
  value = (int)temp;                 // Converte o valor para um inteiro
  return value;                      // Retorna o valor da leitura
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
float pwm(signed int potencia)
{
  float temp = ((float)pwm_period / 100) * potencia; // Calcula o valor do duty cycle do PWM
  return temp;                                       // Retorna o valor do duty cycle
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
void comtroleMotor(int duty, setpoit)
{
  static int velocidadeManual = 0, limiteVelocidade = 150; // Vari�veis est�ticas para controle da velocidade manual

  if (sobe && !Limit_superior) // Verifica se o bot�o de subir est� pressionado e se n�o atingiu o limite superior
  {
    funcao = 1;                                // Define a fun��o como subir
    set_pwm1_duty(velocidadeManual);           // Define o duty cycle do PWM como a velocidade manual
    inverteMotor;                              // Inverte o sentido do motor
    if (velocidadeManual < limiteVelocidade)   // Verifica se a velocidade manual est� abaixo do limite m�ximo
      velocidadeManual = velocidadeManual + 5; // Aumenta a velocidade manual em 5
  }
  else if (desce && !Limit_inferior) // Verifica se o bot�o de descer est� pressionado e se n�o atingiu o limite inferior
  {
    funcao = 2;                                // Define a fun��o como descer
    set_pwm1_duty(velocidadeManual);           // Define o duty cycle do PWM como a velocidade manual
    if (velocidadeManual < limiteVelocidade)   // Verifica se a velocidade manual est� abaixo do limite m�ximo
      velocidadeManual = velocidadeManual + 5; // Aumenta a velocidade manual em 5
  }
  else
  {
    if (liga && habilita && !Limit_inferior) // Verifica se os bot�es de ligar e habilitar est�o pressionados e se n�o atingiu o limite inferior
    {
      if (!setpoit)        // Verifica se o setpoint � igual a 0
        funcao = 0;        // Define a fun��o como parar
      desinverteMotor;     // Desinverte o sentido do motor
      set_pwm1_duty(duty); // Define o duty cycle do PWM como o valor do controle
    }
    else
    {
      set_pwm1_duty(0); // Define o duty cycle do PWM como 0
      potencia = 0;     // Define a pot�ncia como 0
      if (funcao == 1)  // Verifica se a fun��o era de subir
      {
        delay_ms(300);   // Pequeno atraso de 300ms
        funcao = 0;      // Define a fun��o como parar
        desinverteMotor; // Desinverte o sentido do motor
      }
    }
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
void contagem(void)
{
  static int flag = 0;      // Vari�vel est�tica para controle da contagem
  //static int16 contadorTmp; // Vari�vel est�tica para armazenar o valor tempor�rio do contador

  if (menu)          // Verifica se o bot�o de menu est� pressionado
    contador = 0; // Define o contador tempor�rio como 0

  if (conte) // Verifica se o bot�o de contagem est� pressionado
  {
    if (!flag)       // Verifica se o flag � 0
     contador++; // Incrementa o contador tempor�rio
    flag = 1;        // Define o flag como 1
  }
  else
    flag = 0; // Define o flag como 0

 // return contadorTmp; // Retorna o valor do contador tempor�rio
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
void escreveMemoria(int16 variavel)
{
  int byteBaixo, byteAlto, endereco = 0; // Vari�veis para armazenar os bytes baixo e alto da vari�vel e o endere�o da mem�ria

  byteBaixo = variavel & 0xFF; // Obt�m o byte baixo da vari�vel
  byteAlto = variavel >> 8;    // Obt�m o byte alto da vari�vel

  write_eeprom(endereco, byteBaixo);    // Escreve o byte baixo na mem�ria EEPROM
  write_eeprom(endereco + 1, byteAlto); // Escreve o byte alto na mem�ria EEPROM

  if (menu) // Verifica se o bot�o de menu est� pressionado
  {
    write_eeprom(endereco, 0);     // Escreve 0 na posi��o de mem�ria
    write_eeprom(endereco + 1, 0); // Escreve 0 na posi��o de mem�ria
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
int16 lerMemoria(void)
{
  int16 memoria;
  int byteBaixo, byteAlto, endereco = 0; // Vari�veis para armazenar os bytes baixo e alto da vari�vel e o endere�o da mem�ria

  byteBaixo = read_eeprom(endereco);    // L� o byte baixo da mem�ria EEPROM
  byteAlto = read_eeprom(endereco + 1); // L� o byte alto da mem�ria EEPROM

  memoria = byteAlto << 8; // Combina os bytes baixo e alto em uma vari�vel de 16 bits
  memoria |= byteBaixo;

  return memoria; // Retorna o valor armazenado na mem�ria EEPROM
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
void modo(int funcao)
{
  switch (funcao)
  {
  case 0:
    lcd_gotoxy(1, 1);                             // Posiciona o cursor na primeira linha, primeira coluna
    printf(lcd_putc, "\fVelocidade %u", setpoit); // Imprime a velocidade no display

    lcd_gotoxy(1, 2);                            // Posiciona o cursor na segunda linha, primeira coluna
    printf(lcd_putc, "Contador %lu ", contador); // Imprime o contador no display
    break;

  case 1:
    printf(lcd_putc, "\fvelocidade %u\n sobe", setpoit); // Imprime a velocidade e a a��o de subir no display
    break;

  case 2:
    printf(lcd_putc, "\fvelocidade %u\n desce", setpoit); // Imprime a velocidade e a a��o de descer no display
    break;

  case 3:
    printf(lcd_putc, "\fvelocidade %u\n limite inferior", setpoit); // Imprime a velocidade e a indica��o de limite inferior no display
    break;

  case 4:
    printf(lcd_putc, "\fvelocidade %u\n desce", setpoit); // Imprime a velocidade e a a��o de descer no display
    break;
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
void atualizaDisplay(void)
{
  static int t = 0; // Vari�vel est�tica para controle de tempo
  t++;              // Incrementa o valor de t
  if (t == 6)       // Verifica se o valor de t � igual a 6
  {
    t = 0;                                                  // Reinicia o valor de t
    lcd_gotoxy(1, 1);                                       // Posiciona o cursor na primeira linha, primeira coluna
    printf(lcd_putc, "\fVelocidade %u", setpoit); // Imprime a velocidade e o duty cycle no display

    lcd_gotoxy(1, 2);                          // Posiciona o cursor na segunda linha, primeira coluna
    printf(lcd_putc, "Contagem %lu ", contador); // Imprime a frequ�ncia no display
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

/*---------------------------------------------------------------------------------------------------
 C3_supermini_uart.ino
 модуль драйвера L298N. Управление по UART1. 
 (Select board:"Lolin C3 Mini"; USB-CDC on Boot:"Enabled"; Upload Speed:"115200")
-----------------------------------------------------------------------------------------------------*/

// UART1 port for communication with the ESP32-S3-CAM master controller
#define RX 5
#define TX 6

#define LED 7 // light

// подключение модуля драйвера L298N
#define IN1 0  // левый мотор (forward)
#define IN2 1  // левый мотор 
#define IN3 3  // правый мотор (forward)
#define IN4 10 // правый мотор 

//  IN1=1/PWM, IN2=0  - прямое вращение
//  IN1=0, IN2=1/PWM  - реверсное вращение
//  IN1=0, IN2=0  - стоп

#define MIN_SPEED 130       // минимальная скорость
#define MAX_SPEED 255       // максимальная скорость
#define XON 0x11
#define XOFF 0x13

const int freq = 5000;      // частота ШИМ
const int res = 8;          // разрешение ШИМ (в битах)   
uint8_t speed = 0,
        command = 0,
        drive_status = 'S'; // состояние привода
String str = "";

// варианты работы моторов:
// вперед
void Forward(uint8_t sp) {
  ledcWrite(IN2, 0);
  ledcWrite(IN4, 0);
  for (int i = 100; i <= sp; i += 1) {
    ledcWrite(IN1, i);
      ledcWrite(IN3, i+2);
    delay(10);
  }
}

// назад
void Backward(uint8_t sp) {
  ledcWrite(IN1, 0);
  ledcWrite(IN3, 0);
  for (int i = 110; i <= sp; i += 1) {
    ledcWrite(IN2, i+3);
    ledcWrite(IN4, i);
    delay(10);
  }
}

// левый подворот
void Left_turn(uint8_t sp) {
  ledcWrite(IN1, 0);    
  ledcWrite(IN4, 0);
  for (int i = 110; i <= sp; i += 1) {
    ledcWrite(IN3, i);
    ledcWrite(IN2, i);
    delay(2);
  }
}

// правый подворот
void Right_turn(uint8_t sp) {
  ledcWrite(IN2, 0);
  ledcWrite(IN3, 0);
//  digitalWrite(IN2, LOW);
//  digitalWrite(IN3, LOW);
  for (int i = 100; i <= sp; i += 1) {
    ledcWrite(IN1, i);
    ledcWrite(IN4, i);
    delay(2);
  }
}

// оба мотора стоп
void Stop() {  
  ledcWrite(IN1, 0);
  ledcWrite(IN2, 0);
  ledcWrite(IN3, 0);
  ledcWrite(IN4, 0);
}

void Blink() {
  digitalWrite(LED, HIGH);
  delay(500);
  digitalWrite(LED, LOW);
  delay(500);
}

void execute() {
    switch(command) {
      case 70:  Forward(speed);     // 'F' вперед 
                drive_status = 'F';
                break;  
      case 66:  Backward(speed);    // 'B' назад
                drive_status = 'B';
                break;
      case 76:  Left_turn(speed);   // 'L' подворот влево
                delay(200);
                if(drive_status == 'F')
                  Forward(speed);
                else if(drive_status == 'B')
                  Backward(speed);
                else if(drive_status == 'S')
                  Stop();
                break; 
      case 82:  Right_turn(speed);  // 'R' подворот вправо
                delay(200);
                if(drive_status == 'F')
                  Forward(speed);
                else if(drive_status == 'B')
                  Backward(speed);
                else if(drive_status == 'S')
                  Stop();
                break; 
      case 83:  Stop();  // 'S' стоп
                drive_status = 'S';
                break;
      case 87:  digitalWrite(LED, HIGH); // 'W' включить фары
                break;   
      case 88:  digitalWrite(LED, LOW);  // 'X' отключить фары
                break;
      case 89:  Blink(); // 'Y' - yes, можно открывать браузер
                Blink();
                break; 
      default:  if((command >= 0x30 ) && (command < 0x38)) { // от 0x30 до 0x37 
                  // вычитание числа 48 изменит диапазон значений с [48-55] в [0-7].
                  command = command - 48;
                  // умножение результата вычитания на 10 изменит диапазон с [0-7] на [0-70].
                  speed = MIN_SPEED + command * 10; // в итоге: speed = [130-200]
                  if(drive_status == 'F')
                    Forward(speed);
                  else if(drive_status == 'B')
                    Backward(speed);
                }
    }
  delay(10);
  command = 0;  
};

void setup() {
  Serial.begin(115200); // UART0 для отладки
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW); // выключить фары
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(TX, OUTPUT);
  pinMode(RX, INPUT);
  
  ledcAttach(IN1, freq, res); // привязка ШИМ к пинам
  ledcAttach(IN2, freq, res); // 
  ledcAttach(IN3, freq, res); //
  ledcAttach(IN4, freq, res); //

  speed = MIN_SPEED;
  Stop();
  Serial.println("RC Car Control");

  Serial1.begin(115200, SERIAL_8N1, RX, TX); // Инициализировать UART1
  delay(10);
  Serial1.write(XOFF);  // запрет отправки команд управления
  while(Serial1.available() > 0) { // если в буфере UART1 есть данные, 
    command = Serial1.read();      // считываем их (очищаем буфер)
  };
  command = 0;
  Serial1.write(XON);   // готовность к выполнению команд управления
  Serial.println("Waiting a control command...");
}

void loop() {
  if(Serial1.available() > 0) { // если в буфере UART1 есть данные
    command = Serial1.read();   // считываем и запоминаем команду (1 символ)
    Serial.print(command);
    Serial1.write(XOFF);  // запрет отправки команд
    execute(); // выполнить команду
    Serial1.write(XON);  // готов  к приему следующей команды
  }
}
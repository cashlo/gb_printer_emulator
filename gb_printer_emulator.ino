#include <odroid_go.h>
#define PIN_SERIAL_CLOCK  4
#define PIN_SERIAL_IN     15
#define PIN_SERIAL_OUT    12

const int packet_byte_magic_1     = 0;
const int packet_byte_magic_2     = 1;
const int packet_byte_command     = 2;
const int packet_byte_compression = 3;
const int packet_byte_length_1    = 4;
const int packet_byte_length_2    = 5;
const int packet_byte_data        = 6;
const int packet_byte_checksum_1  = 7;
const int packet_byte_checksum_2  = 8;
const int packet_byte_alive       = 9;
const int packet_byte_status      = 10;




byte serial_data;
int data_counter = 8;
int line;

int awaiting_byte = packet_byte_magic_1;
int command;
bool commpression;
int data_length_left;
int checksum;
int data_to_send;

void print_status(byte input_byte){
      GO.lcd.setCursor(0,line*10);
      String output;
      output = output + " Command: " + command + " Compression: " + commpression + "Length: " + data_length_left + " Input: " + input_byte;
      GO.lcd.print(output);
      line++;
  }

void process_byte(byte input_byte) {
  switch (awaiting_byte) {
    case packet_byte_magic_1:
      if (input_byte != 0x88) return;
      break;
    case packet_byte_magic_2:
      if (input_byte != 0x33) {
        awaiting_byte = packet_byte_magic_1;
        return;
      }
      break;
    case packet_byte_command:
      command = input_byte;
      break;
    case packet_byte_compression:
      commpression = input_byte;
      break;
    case packet_byte_length_1:
      data_length_left = input_byte;
      break;
    case packet_byte_length_2:
      data_length_left |= input_byte << 8;
      if (data_length_left == 0) {
          awaiting_byte = packet_byte_checksum_1;
          return;
      }
      break;
    case packet_byte_data:
      //TODO
      data_length_left--;
      if (data_length_left == 0) {
          break;
      }
      return;
    case packet_byte_checksum_1:
      checksum = input_byte;
      break;
    case packet_byte_checksum_2:
      checksum |= input_byte << 8;
      //TODO
      data_to_send = 0x81;
      break;
    case packet_byte_alive:
      break;
    case packet_byte_status:
      awaiting_byte = packet_byte_magic_1;
      print_status(input_byte);
      return;
  }
  awaiting_byte++;
}

void IRAM_ATTR send_bit(){
  digitalWrite(PIN_SERIAL_OUT, (data_to_send&0x80)>>7);
  data_to_send <<= 1;
}

void IRAM_ATTR read_bit(){
  byte incoming_bit = digitalRead(PIN_SERIAL_IN);
  serial_data <<= 1;
  serial_data |= incoming_bit;
  data_counter--;
  if(data_counter == 0){
    process_byte(serial_data);
    //GO.lcd.print(serial_data, HEX);
    //line++;
    data_counter = 8;
    serial_data = 0;
  }
}

void IRAM_ATTR serial_clock_handler(){
  if(digitalRead(PIN_SERIAL_CLOCK)){
    read_bit();
  }else{
    send_bit();
  }
} 
void setup() {
  // put your setup code here, to run once:
  GO.begin();

  pinMode(PIN_SERIAL_OUT, OUTPUT);
  digitalWrite(PIN_SERIAL_OUT, LOW);
  attachInterrupt(digitalPinToInterrupt(PIN_SERIAL_CLOCK), serial_clock_handler, CHANGE);
}

void loop() {
  // put your main code here, to run repeatedly:
}

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

void print_byte(byte input_byte){
  GO.lcd.setCursor(100,line*10);
  GO.lcd.print(input_byte, HEX);
  line++;
}

void print_string(String input_byte){
  GO.lcd.setCursor(100,line*10);
  GO.lcd.print(input_byte);
  line++;
}

int draw_pos;
uint32_t palette[] = {0xFFFF, 0xC618, 0x7BEF, 0x0000};
byte print_data[6400];
int print_data_index;
bool drawing_print_data = false;

int y_scroll;
int display_scale = 2;

void draw_data() {
  GO.lcd.clear();
  int i = 0;
  while (i < print_data_index-1){
    byte byte_1 = print_data[i];
    byte byte_2 = print_data[i+1];
    int tile_col = (i/2/8)%20;
    int tile_row = i/(20*2*8)-y_scroll;

    int y_tile_pos = i/2%8;
    
    int j = 7;
    while ( j >= 0 ){
        int x_tile_pos = j;

        uint32_t pixel_color = palette[(byte_1&1) + (byte_2&1)*2];
        
        GO.lcd.drawRect((x_tile_pos+tile_col*8)*display_scale,
          (y_tile_pos+tile_row*8)*display_scale,
          display_scale,
          display_scale,
          pixel_color);
          
        byte_1 >>= 1;
        byte_2 >>= 1;
        j--;
    }
    i += 2;
  }
  drawing_print_data = false;
  //print_data_index = 0;
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
      print_data[print_data_index] = input_byte;
      print_data_index++;
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
      if (command == 0x2) {
        drawing_print_data = true;
      }
      //print_status(input_byte);
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
  GO.begin(9600);

  pinMode(PIN_SERIAL_OUT, OUTPUT);
  digitalWrite(PIN_SERIAL_OUT, LOW);
  attachInterrupt(digitalPinToInterrupt(PIN_SERIAL_CLOCK), serial_clock_handler, CHANGE);
}

void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void handle_buttons(){
    if(GO.JOY_Y.isAxisPressed() == 2){
        if(y_scroll <= 0) return;
        y_scroll--;
        draw_data();
    }
    if(GO.JOY_Y.isAxisPressed() == 1){
        y_scroll++;
        draw_data();
    }

    if(GO.JOY_X.isAxisPressed() == 2){
        if(display_scale <= 1) return;
        display_scale--;
        draw_data();
    }
    if(GO.JOY_X.isAxisPressed() == 1){
        if(display_scale >= 2) return;
        display_scale++;
        draw_data();
    }
        
    if(GO.BtnB.isPressed() == 1){
        GO.lcd.clear();
        print_data_index = 0;
        awaiting_byte = packet_byte_magic_1;
    }

    if(GO.BtnA.isPressed() == 1){
      GO.lcd.clear();

      Serial.print("Initializing SD card...");
    
      if (!SD.begin()) {
        Serial.println("initialization failed!");
      }
      Serial.println("initialization done.");
  
      print_data_index = 0;
      awaiting_byte = packet_byte_magic_1;
    }
}

void loop() {
  // put your main code here, to run repeatedly:
  GO.update();
  if(drawing_print_data) draw_data();
  delay(100);
  handle_buttons();
}

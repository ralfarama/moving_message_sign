
int PIN_CSG2B = A0;
int PIN_CSSA  = A1;
int PIN_CSSB  = A2;
int PIN_CSSC  = A3;
int PIN_CSG2A = A4;

int PIN_DBUS0 = 2;
int PIN_DBUS1 = 3;
int PIN_DBUS2 = 4;
int PIN_DBUS3 = 5;
int PIN_DBUS4 = 6;
int PIN_DBUS5 = 7;
int PIN_DBUS6 = 8;
int PIN_DBUS7 = 9;

void fullreset(void) {
  // zero out the data bus and pull the reset line low to clear registers
  digitalWrite(PIN_DBUS0,LOW);
  digitalWrite(PIN_DBUS1,LOW);
  digitalWrite(PIN_DBUS2,LOW);
  digitalWrite(PIN_DBUS3,LOW);
  digitalWrite(PIN_DBUS4,LOW);
  digitalWrite(PIN_DBUS5,LOW);
  digitalWrite(PIN_DBUS6,LOW);
  digitalWrite(PIN_DBUS7,LOW);  // reset line low
  delay(10) ;           // keep it low for .01 sec
  digitalWrite(PIN_DBUS7,HIGH); // pull up reset line
}

void setup() {
  // put your setup code here, to run once:

  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);

  // 74LS138 demultiplexer chip select control signal outputs
  pinMode(PIN_CSG2B, OUTPUT); // 74LS138 Pin 5 - /G2B
  pinMode(PIN_CSSA, OUTPUT); // 74LS138 Pin 1 - A
  pinMode(PIN_CSSB, OUTPUT); // 74LS138 Pin 2 - B
  pinMode(PIN_CSSC, OUTPUT); // 74LS138 Pin 3 - C
  pinMode(PIN_CSG2A, OUTPUT); // 74LS138 Pin 4 - /G2A
  // 74LS138 outputs (Y=1*A+2*B+4*C):
  // Y2 - 74LS259 IC11 /G input
  // Y3 - 74LS259 IC13 /G input
  // Y4 - 74LS373 IC4  CP input
  // Y5 - 74LS373 IC5  CP input
  // Y6 - 74LS373 IC10 CP input
  // Y7 - 74LS373 IC12 CP input
  
  // data bus outputs
  // each line goes to two 74LS259 latches and four 74LS273 registers
  pinMode(PIN_DBUS0, OUTPUT); // LS273 D0  (Pin  3) ; LS259 D   (Pin 13)
  pinMode(PIN_DBUS1, OUTPUT); // LS273 D1  (Pin  4) ; LS259 S0  (Pin  1)
  pinMode(PIN_DBUS2, OUTPUT); // LS273 D2  (Pin  7) ; LS259 S1  (Pin  2)
  pinMode(PIN_DBUS3, OUTPUT); // LS273 D3  (Pin  8) ; LS259 S2  (Pin  3)
  pinMode(PIN_DBUS4, OUTPUT); // LS273 D4  (Pin 13)
  pinMode(PIN_DBUS5, OUTPUT); // LS273 D5  (Pin 14)
  pinMode(PIN_DBUS6, OUTPUT); // LS273 D6  (Pin 17)
  // this line acts as a reset line to the latches and registers
  pinMode(PIN_DBUS7, OUTPUT); // LS273 /MR (Pin  1) ; LS259 /CLR (Pin 15)

  // set initial state to LS138 disabled, all other lines low
  digitalWrite(PIN_CSG2B,HIGH);
  digitalWrite(PIN_CSSA,LOW);
  digitalWrite(PIN_CSSB,LOW);
  digitalWrite(PIN_CSSC,LOW);
  digitalWrite(PIN_CSG2A,HIGH);
  fullreset();
}

void strobepixel(int row, int bank, int bcol, int ltime, int htime, int reps) {
  fullreset();
  // set latch
  int latch_i, bit_i;
  int bankbit0, bankbit1,bankbit2;

  if ( (bank+2) & 1 ) { bankbit0 = HIGH; } else { bankbit0 = LOW; }
  if ( (bank+2) & 2 ) { bankbit1 = HIGH; } else { bankbit1 = LOW; }
  if ( (bank+2) & 4 ) { bankbit2 = HIGH; } else { bankbit2 = LOW; }

  digitalWrite(PIN_CSG2B,HIGH); // disable all LS138 outputs
  digitalWrite(PIN_CSG2A,HIGH); // disable all LS138 outputs
  // disable the LS138 outputs to prepare for LS259 latch selection  
  for ( latch_i = 0; latch_i < 2; latch_i++ ) {
    for ( bit_i = 0; bit_i < 8; bit_i++ ) {
      int s0val, s1val, s2val;
      if ( bit_i & 1 ) { s0val = HIGH; } else { s0val = LOW; }
      if ( bit_i & 2 ) { s1val = HIGH; } else { s1val = LOW; }
      if ( bit_i & 4 ) { s2val = HIGH; } else { s2val = LOW; }
      // disable LS138 outputs prior to setting signals
      digitalWrite(PIN_CSG2B,HIGH);
      digitalWrite(PIN_CSG2A,HIGH);
      // set LS138 signals to perform latch select to enable desired column
      digitalWrite(PIN_CSSC,HIGH); // set high select bit of LS138 
      digitalWrite(PIN_CSSB,HIGH); // set middle select bit of LS138
      digitalWrite(PIN_CSSA,latch_i); // low select bit of LS138, selecting latch
      digitalWrite(PIN_DBUS0,HIGH); // disable latch clear signal
      digitalWrite(PIN_DBUS6,s0val);
      digitalWrite(PIN_DBUS5,s1val);
      digitalWrite(PIN_DBUS4,s2val);
      // latch logic is inverted, to light up a column, the appropriate
      // latch bit should be 0, and 1 turns off a column
      if ( bit_i == ( bcol & 7 ) && ( latch_i == ( bcol >> 3 )  ) ) {
        digitalWrite(PIN_DBUS7,LOW);
      } else {
        digitalWrite(PIN_DBUS7,HIGH);
      }
      digitalWrite(PIN_CSG2B,LOW);
      digitalWrite(PIN_CSG2A,LOW);
      delayMicroseconds(50);
    }
  }
  // disable LS138 outputs before moving on
  digitalWrite(PIN_CSG2B,HIGH);
  digitalWrite(PIN_CSG2A,HIGH);

  // set LS138 select bits to choose desired LS273 register
  digitalWrite(PIN_CSSA,bankbit0);
  digitalWrite(PIN_CSSB,bankbit1);
  digitalWrite(PIN_CSSC,bankbit2);
  // set data but bits to select the right row
  int i;
  digitalWrite(PIN_DBUS0,HIGH); // clear 74LS273 /MR input
  // PIN_DBUS1 = bottom row, PIN_DBUS7 = top row
  for ( i = 0; i < 7; i++ ) {
    int portnum = i + 3;
    if ( row == i ) {
      digitalWrite(portnum,HIGH);
    } else {
      digitalWrite(portnum,LOW);
    }
  }
  // finally, re-enable LS138 to effect the register load
  digitalWrite(PIN_CSG2B,LOW);
  digitalWrite(PIN_CSG2A,LOW);
  delay(1);
  digitalWrite(PIN_CSG2B,HIGH);
  digitalWrite(PIN_CSG2A,HIGH);
  delay(1);
// re-enable the LS138 prior to strobing the pixel
  digitalWrite(PIN_CSG2B,LOW);
  digitalWrite(PIN_CSG2A,LOW);
  delay(1);
  // disable the LS138 outputs to prepare for LS259 latch selection
  int rep;
  for ( rep = 0; rep < reps; rep++ ) {
    digitalWrite(PIN_DBUS0,HIGH); // set D to load a 1 bit into the desired latch
    delay(htime);
    digitalWrite(PIN_DBUS0,LOW);
    delay(ltime);
  }
  digitalWrite(PIN_CSG2B,HIGH);
  digitalWrite(PIN_CSG2A,HIGH);  
}

void loop() {
  // put your main code here, to run repeatedly:
  int bank, row, col;

  for ( row = 0; row < 7; row++ ) {
    for ( bank = 0; bank < 4; bank++ ) {
      for ( col = 0; col < 15; col++ ) {
        strobepixel(row,bank,col,1,1,1);
        if ( col % 2 == 1 ) {
          digitalWrite(LED_BUILTIN,HIGH);
        } else {
          digitalWrite(LED_BUILTIN,LOW);
        }
      }
    }
  }
}

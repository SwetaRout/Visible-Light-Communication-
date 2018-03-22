int analogPin = 0;
int digitalPin = 3;
int val = 0;
#define numberOfBytes 50
byte ledByte[numberOfBytes] = { 0 };
int i = 0;
int j = 0;
int k = 0;

#define AnalogThreshold 50 //adc counts
#define BitDelay 471 //in microseconds
#define initialDelay 1750

// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

void setup() 
{
  Serial.begin(115200);
  pinMode(digitalPin, OUTPUT);

  // set prescale to 16
  sbi(ADCSRA,ADPS2) ;
  cbi(ADCSRA,ADPS1) ;
  cbi(ADCSRA,ADPS0) ;
}

void nopDelay(int numberOfNops)
{
  for(int i = 0; i < numberOfNops; i++)
  {
    __asm__("nop\n\t"); //16 ns
  }
}

void SendBytes()
{
  Serial.write(ledByte, numberOfBytes);
}

void SwapByte(byte *ledByte)
{
  byte reversedByte;

  for (k = 0; k < 8; k++)
  {
     if((*ledByte & (1 << k)))
        reversedByte |= 1 << ((8 - 1) - k);  
  }

  *ledByte = reversedByte;
}

void loop() 
{ 
  while(analogRead(analogPin) < AnalogThreshold);
  digitalWrite(digitalPin, HIGH);
  while(analogRead(analogPin) > AnalogThreshold); //receive transmission start signal
  digitalWrite(digitalPin, LOW);
  while(analogRead(analogPin) < AnalogThreshold);
  delayMicroseconds(initialDelay);

  for(i = 0; i < numberOfBytes; i++)
  {
      for(j = 0; j < 8; j++)
      {
         val = analogRead(analogPin);
         digitalWrite(digitalPin, HIGH);

         ledByte[i] = ledByte[i] << 1;
         ledByte[i] |= (val < AnalogThreshold);

         delayMicroseconds(BitDelay);
         digitalWrite(digitalPin, LOW);
     }
  }

  for(i = 0; i < numberOfBytes; i++)
  {
    SwapByte(&ledByte[i]); 
  }

  SendBytes();
    
  delay(100);
}

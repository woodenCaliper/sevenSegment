#include "Charlieplexing.h"
#include <MsTimer2.h>

#ifndef MAX_7SEGMENT
#define MAX_7SEGMENT 10
#endif

#ifndef CHARLIEPLEXING_MAX_USE_PIN
#define CHARLIEPLEXING_MAX_USE_PIN 20
#endif

#ifdef min
#ifdef max
#define clamp(num, minNum, maxNum) min(max(num, minNum), maxNum)
#endif
#endif

class SevenSegDriveClass{
  public:

  const static uint16_t UNUSE_LED=0xffff;
  uint8_t numOf7Segment;

  struct Segment{
    uint16_t ledIds[8];
  } segState[MAX_7SEGMENT];

  CharlieplexingClass* cc;

  //abcdefg dp
  const uint8_t numToAbcd[10] =   { 0b11111100,   //0
                                    0b01100000,   //1
                                    0b11011010,   //2
                                    0b11110010,   //3
                                    0b01100110,   //4
                                    0b10110110,   //5
                                    0b00111110,   //6
                                    0b11100000,   //7
                                    0b11111110,   //8
                                    0b11100110    //9
                                  };
  const uint8_t spaceToSeg     =  0b00000000;
  const uint8_t dotToSeg       =  0b00000001;
  const uint8_t dashToSeg      =  0b00000010;
  const uint8_t underScoreToSeg=  0b00010000;

  uint8_t modeCommon, modeResistor, modeLightWay;

  /*
    usually connectiong
    a,b,c,d,e,f,g,dp == LED ID
    led[] = {a,b,c,d,e,f,g,dp}
    or
    if charlieplexing
    firstSegPins8, secondSegPins8
    leds[] = {a,b,c,d,e,f,g,dp, a,b,c,d,e,f,g,dp, a,b,c,d,e,f,g,dp}
  */
/*
abcdefgdotState--------->>

   b d f dp
  a|c|e|g|
  ||||||||
0bxxxxxxxx

----------------------- ------------ ------------ ------------
7segment led names     |  aaa       |  aaa       |  aaa       |
                       | f   b      | f   b      | f   b      |
                       | f   b      | f   b      | f   b      |
                       |  ggg       |  ggg       |  ggg       |
                       | e   c      | e   c      | e   c      |
                       | e   c      | e   c      | e   c      |
                       |  ddd   dp  |  ddd   dp  |  ddd   dp  |
----------------------- ------------ ------------ ------------
*/
  SevenSegDriveClass(uint8_t numOfSevenSegment, const uint8_t commonPins[], const uint8_t digitPins[]){
    uint8_t usePins[CHARLIEPLEXING_MAX_USE_PIN];
    uint8_t usePinsLen=0;
    for(uint8_t i=0; i<numOfSevenSegment; i++){
      bool flag = true;
      for(uint8_t j=0; j<usePinsLen; j++){
        if(usePins[j] == commonPins[i]){
          flag = false;
          break;
        }
      }
      if(flag){
        usePins[usePinsLen] = commonPins[i];
        usePinsLen++;
      }
    }
    for(uint8_t i=0; i<numOfSevenSegment*8; i++){
      bool flag = true;
      for(uint8_t j=0; j<usePinsLen; j++){
        if(usePins[j] == digitPins[i]){
          flag = false;
          break;
        }
      }
      if(flag){
        usePins[usePinsLen] = digitPins[i];
        usePinsLen++;
      }
    }
    numOf7Segment = numOfSevenSegment;

    cc = new CharlieplexingClass(usePins, usePinsLen);

    for(uint8_t s=0; s<numOf7Segment; s++){
      for(uint8_t d=0; d<8; d++){
        segState[s].ledIds[d] = digitPins[d]==-1 ? UNUSE_LED : getLedId(commonPins[s], digitPins[8*s+d]);
      }
    }
  }

  void addLightingLed(uint16_t ledsId[], byte numOfLeds){
    cc->addLedState(ledsId, numOfLeds);
  }
  unsigned int getLedId(byte anodePin, byte cathodePin){
    return cc->getLedId(anodePin, cathodePin);
  }
  void update(){
    cc->updateLightingState();
  }
  void setLedStateFromSegState(uint8_t abcdefgdp, int8_t segmentNumber){
    segmentNumber = clamp(segmentNumber, -numOf7Segment, numOf7Segment-1);//範囲内に収める
    if(segmentNumber<0){//負の場合、末尾から数える
      segmentNumber = numOf7Segment + segmentNumber;
    }
    for(uint8_t d=0; d<8; d++){
      cc->addLedState(segState[segmentNumber].ledIds[d], (0b10000000>>d) & abcdefgdp);
    }
  }
  // void clear(){
  //   for(uint8_t s=0; s<numOf7Segment; s++){
  //     segState[s].lightState = 0x00;  //初期化
  //   }
  // }

  // public:
  // void clearLedState(){
  //   cc->resetLedState();
  // }

  // void setAll(){
  //   uint16_t lightLedId[100];
  //   uint8_t ledIdsLen;
  //   for(uint8_t i=0; i<8*numOf7Segment; i++){
  //     segState[i].lightState = 0xff;
  //   }
  //   getLedIdFromSegState(&lightLedId[0], &ledIdsLen);
  //   cc->setLedState(lightLedId, ledIdsLen);
  // }

  void setLight(int32_t num, int8_t dispEndSeg, uint8_t maxDispDigit=255){
    dispEndSeg = clamp(dispEndSeg, -numOf7Segment, numOf7Segment-1);//範囲内に収める
    if(dispEndSeg<0){//負の場合、末尾から数える
      dispEndSeg = numOf7Segment + dispEndSeg;
    }
    if(maxDispDigit == 0 || dispEndSeg+1 < maxDispDigit){
      maxDispDigit = dispEndSeg+1;
    }
    for(uint8_t i=0; i<maxDispDigit; i++){
      setLedStateFromSegState(spaceToSeg, dispEndSeg-i);//無表示に初期化
    }
    uint8_t canNumDispDigit;
    canNumDispDigit = 0<=num ? maxDispDigit : maxDispDigit-1;
    if(canNumDispDigit==0){
      num = clamp(num, 0, 9);
    }
    else if(canNumDispDigit<=9){
      int32_t maxNum=0;
      for(uint8_t i=0; i<canNumDispDigit; i++){
        maxNum = maxNum*10+9; //99...9
      }
      num = clamp(num, -maxNum, maxNum);
    }

    uint8_t numParts[20];
    uint8_t numPartsLen;
    numPartsLen = disassemblyUint(abs(num), numParts);

    //マイナスの表示
    if(num<0){
      setLedStateFromSegState(dashToSeg, dispEndSeg-numPartsLen);
    }
    //数値の表示
    for(uint8_t i=0; i<numPartsLen; i++){
      setLedStateFromSegState(numToAbcd[numParts[i]], dispEndSeg-numPartsLen+1+i);
    }
  }

  void setLight(double num, int8_t dispEndSeg, uint8_t maxDispDigit=255){
    dispEndSeg = clamp(dispEndSeg, -numOf7Segment, numOf7Segment-1);//範囲内に収める
    if(dispEndSeg<0){//負の場合、末尾から数える
      dispEndSeg = numOf7Segment + dispEndSeg;
    }
    if(maxDispDigit == 0 || dispEndSeg+1 < maxDispDigit){
      maxDispDigit = dispEndSeg+1;
    }
    for(uint8_t i=0; i<maxDispDigit; i++){
      setLedStateFromSegState(spaceToSeg, dispEndSeg-i);//無表示に初期化
    }

    uint8_t canNumDispDigit;
    canNumDispDigit = 0.0<=num ? maxDispDigit : maxDispDigit-1;

    if(canNumDispDigit==0){
      num = clamp(num, 0.0, 9.0);
      canNumDispDigit=1;
    }
    else{
      double maxNum=0;
      for(uint8_t i=0; i<canNumDispDigit; i++){
        maxNum = maxNum*10+9; //99...9
      }
      num = clamp(num, -maxNum, maxNum);
    }

    uint8_t numParts[20];
    uint8_t dpPos;
    disassemblyFloat(num, canNumDispDigit, numParts, &dpPos);

    uint8_t dispStartSeg = dispEndSeg-canNumDispDigit;
    //マイナスの表示
    if(num<0.0){
      setLedStateFromSegState(dashToSeg, dispStartSeg);
    }
    //数値の表示
    for(uint8_t i=0; i<canNumDispDigit; i++){
      if(i==dpPos && dpPos!=canNumDispDigit-1){
        setLedStateFromSegState(numToAbcd[numParts[i]]|dotToSeg, dispStartSeg+1+i);
      }
      else{
        setLedStateFromSegState(numToAbcd[numParts[i]], dispStartSeg+1+i);
      }
    }
  }

  protected:
  uint8_t disassemblyFloat(double num, uint8_t returnDigit, uint8_t* numParts, uint8_t* dpPos){
    num=abs(num);
    uint8_t digit=0;
    uint32_t n = (uint32_t)num;
    do{//整数部の桁計算,0でも1桁
      digit++;
      n /=10;
    }while(n);
    *dpPos = digit-1;

    uint32_t numUint = (uint32_t)num;
    double numFloat = num-numUint;
    for(uint8_t i=0; i<returnDigit; i++){
      if(digit>0){//整数部
        uint32_t b = pow(10, digit-1)+0.5;
        numParts[i] = numUint/b;
        numUint -= numParts[i]*b;
        digit--;
      }
      else{//少数部
        numFloat *= 10;
        numParts[i] = (uint32_t)numFloat;
        numFloat -= numParts[i];
      }
    }
    return returnDigit;
  }

  uint8_t disassemblyUint(uint32_t num, uint8_t* parts){
    const uint8_t MAX_DIGIT = 10;
    uint8_t numParts[MAX_DIGIT];
    uint8_t len=0;
    do{
      numParts[len++] = num%10;
      num /= 10;
    }while(num);
    for(uint8_t i=0; i<len; i++){
      parts[i] = numParts[len-i-1];
    }
    return len;
  }
};




const uint8_t SEG_NUM = 9;
const uint8_t COMMON_PINS[] = {2,3,4,5,6,7,8,9,10};
const uint8_t DIGIT_PINS[] = {  3,4,5,6,7,8,9,10,
                                2,4,5,6,7,8,9,10,
                                3,2,5,6,7,8,9,10,
                                3,4,2,6,7,8,9,10,
                                3,4,5,2,7,8,9,10,
                                3,4,5,6,2,8,9,10,
                                3,4,5,6,7,2,9,10,
                                3,4,5,6,7,8,2,10,
                                3,4,5,6,7,8,9,2
                              };

SevenSegDriveClass ssd(SEG_NUM, COMMON_PINS, DIGIT_PINS);

void fire(){
  ssd.update(); ///120us
}

void setup() {
  Serial.begin(9600);
  Serial.println("start2");
  MsTimer2::set(2, fire);
  MsTimer2::start();
  ssd.setLedStateFromSegState(ssd.underScoreToSeg, -3);
  ssd.setLedStateFromSegState(ssd.underScoreToSeg, -6);
}

void loop(){
  int32_t secs = (uint32_t)(millis()/1000.0)%60;
  int32_t mins = (uint32_t)(millis()/1000.0/60.0)%60;
  int32_t hours = (uint32_t)(millis()/1000.0/60.0/24.0)%24;
  ssd.setLight(secs, -1, 2);
  ssd.setLight(mins, -4, 2);
  ssd.setLight(hours, -7, 2);
  delay(500);
}


  // uint32_t t0=0, t2=0;
  // ssd.lightSetI2(-23456789, 10, 10);

  // // ssd.setLedStateFromSegState(0xff, 0);
  // while(1){
  //   while(micros()-t0 < 2000);  //wait 2000us
  //   t0=micros();
  //   // Serial.println("a");

  //   ssd.update();
  // }


    // if(millis()-t2 > 100){
    //   t2=millis();
    //   ssd.lightSetI2(millis(), 8);
    // }
// }

// void loop() {
//   SevenSegDriveClass ssd(SEG_NUM, COMMON_PINS, DIGIT_PINS);
//   ssd.lightSetF(30000, 9);
//   uint32_t t0=0, t2=0;
//   while(1){
//     while(micros()-t0 < 2000);  //wait 2000us
//     t0=micros();

//     ssd.update();

//     if(millis()-t2 > 100){
//       t2=millis();
//       ssd.lightSetF(micros()/1000.0, 9);
//     }
//   }
// }

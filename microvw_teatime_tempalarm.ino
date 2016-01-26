#include <font5x7.h>
#include <font8x16.h>
#include <fontlargenumber.h>
#include <MicroView.h>
#include <space01.h>
#include <space02.h>
#include <space03.h>
#include <OneWire.h>
#include <SimpleTimer.h>

/* HEJ */
/*
   TEA TIMER WITH TEMPERATURE ALARM
   Designed for use with MicroView Arduino (http://www.microview.io)
   and DS18B20 Waterproof Pre-Wired Thermometer (https://www.adafruit.com/products/381)

   @author Fredrik Nordström (fredrik.nordstrom@gmail.com)
*/

//variables for input/output pins
int snd = A5; //output for buzzer on analog pin 5
int DS18S20_Pin = 6; //thermometer input signal pin on digital 6
int UP = A0; //pin for UP button on analog 0
int DOWN = A1; //input for DOWN button on analog pin 1
int LEFT = A3; //input for LEFT button on analog pin 3
int RIGHT = A2; //input for RIGHT button on analog pin 2
int OK = A4; //input for OK button on analog pin 4
int SET = 2; //input for SET button on digital pin 2


//other variables
boolean exitMenu = false;
int goalTemp = 0; //the default goal temperature
int newGoalTemp = 0; //temporary holder for new goal temperature while changing setting
boolean tempReached = false; //to avoid sounding the alarm more than once
long waitingTime = 350000; //value used in simulating delay() inside a method
int delayCounter = 0; //value used in simulating delay() inside a method
SimpleTimer timer; //a timer object for the countdown timer
int timerID = 0; //the ID for the countdown timer
SimpleTimer heatWaveTimer; //a timer object for the heat wave animation
int heatWaveTimerID = 1; //the ID for the timer used to "animate" the heat waves
long timerTotalInSeconds = 0; //value will be set later and used as input to the actual timer object
int timerMinutes = 4; //default countdown timer minutes
int timerSeconds = 0; //default countdown timer seconds
boolean timerIsOn = false; //boolean to check in the loop if there is a running countdown timer
boolean showTempScreenOnce = false; //boolean to make the static temperature text update only once (and not every loop), otherwise the temp readings won't show for some strange reason
boolean noInterrupt = false; //boolean for ignoring the "Set temperature" dialog when in timer mode
boolean soundOff = false; //boolean for setting the alarm sound
boolean heatWavesFlipped = false; //boolean used to determine if the heat waves are flipped or not during the animation sequence

OneWire ds(DS18S20_Pin);  // initialize thermometer on digital pin 6

void setup()
{

  attachInterrupt(0, setTemperature, FALLING);
  timer.disable(timerID);
  pinMode(snd, OUTPUT); //setting the buzzer snd pin as output
  pinMode(UP, INPUT); //setting the UP button pin as input
  pinMode(DOWN, INPUT); //setting the DOWN button pin as input
  pinMode(LEFT, INPUT); //setting the LEFT button pin as input
  pinMode(RIGHT, INPUT); //setting the RIGHT button pin as input
  pinMode(OK, INPUT); //setting the OK button pin as input
  pinMode(SET, INPUT); //setting the SET button pin as input

  uView.begin(); //initiate MicroView display
  uView.clear(PAGE); //clear the display

  heatWaveTimerID = heatWaveTimer.setInterval(1300, animateHeatWaves);
  heatWaveTimer.enable(heatWaveTimerID);

  startMenu(); //show the start menu

}

void loop() {

  uView.invert(false); //uninvert the screen if the program was interrupted while in inverted state
  
  if (!tempReached && !timerIsOn /*&& goalTemp != 0*/) { //only show the animated heat waves if the desired temperature hasn't been met [and the temperature alarm is set]

    heatWaveTimer.run();

  }


  if (timerIsOn == true) {

    timer.run();

  } else {

    if (showTempScreenOnce == false) {
      uView.clear(PAGE);
      showTempScreen();
      showTempScreenOnce = true;
    }

    float temperature = getTemp(); //get temperature from the thermometer

    if (soundOff) {

      wait();
      drawNoSoundIconTemperature();

    }

    uView.setCursor(16, 17);
    uView.setColor(0);
    uView.print(temperature);
    uView.display();
    uView.setColor(1);

    if (temperature > 0 && temperature <= goalTemp && !tempReached) {

      tempReached = true;

      wait();
      wait();

      if (soundOff == false) {
        digitalWrite(snd, HIGH);
        uView.invert(true);
        delay(500);
        digitalWrite(snd, LOW);
        uView.invert(false);
      }

      while (digitalRead(OK) == HIGH) {

        uView.invert(true);
        delay(500);
        uView.invert(false);
        delay(500);

      }

      soundOff = false;

    }
  }

}



void startMenu() {

  uView.clear(PAGE);

  while (exitMenu == false) {

    uView.setCursor(0, 0);
    uView.print("-SELECT-");
    uView.setCursor(0, 15);
    uView.print("Temp.");
    //up arrow
    uView.pixel(37, 17);
    uView.lineH(36, 18, 3);
    uView.lineH(35, 19, 5);

    uView.setCursor(0, 25);
    uView.print("Timer");

    uView.lineH(35, 27, 5);
    uView.lineH(36, 28, 3);
    uView.pixel(37, 29);

    uView.display();

    if (digitalRead(DOWN) == HIGH) {
      noInterrupt = true;
      setTimerMinutes();
      exitMenu = true;

    } else if (digitalRead(UP) == HIGH) {
      showTempScreen();
      exitMenu = true;
    }

  }



}

void setTimerMinutes() {

  boolean waitToChange = false; //boolean for prevention of value changes while waiting

  uView.clear(PAGE);

  //waiting to avoid too fast reading of DOWN button from previous dialog
  wait();
  wait();

  uView.setCursor(0, 40);
  uView.print(timerMinutes);
  uView.display();

  while (digitalRead(OK) == HIGH) { //run the enter minutes dialog until the OK button is pressed

    enterNewTimerMinutesDialog();

    if (digitalRead(UP) == HIGH && waitToChange == false) {

      waitToChange = true;

      if (timerMinutes >= 9) { //There is NO point in letting your tea steep for more than 9 minutes, that is already waaaaay too long (ok, maybe for some oolongs, but that is yet to be supported :)
        timerMinutes = 9;
      } else {
        timerMinutes += 1;
      }

      uView.setCursor(0, 40);
      uView.print(timerMinutes);
      uView.display();
      wait(); //waiting to avoid too fast changes in value

      waitToChange = false;

    } else if (digitalRead(DOWN) == HIGH && waitToChange == false) {

      waitToChange = true;

      if (timerMinutes <= 0) {
        timerMinutes = 0;
      } else {
        timerMinutes -= 1;
      }

      uView.setCursor(0, 40);
      uView.print(timerMinutes);
      uView.display();
      wait(); //waiting to avoid too fast changes in value

      waitToChange = false;
    }
  }

  timerTotalInSeconds = timerMinutes * 60;

  setTimerSeconds(); //calling the method for setting seconds

}

void enterNewTimerMinutesDialog() {
  uView.setCursor(0, 0);
  uView.print("Enter");
  uView.setCursor(0, 12);
  uView.print("minutes:");
  uView.display();
}

void setTimerSeconds() {
  boolean waitToChange = false; //boolean for prevention of value changes while waiting

  uView.clear(PAGE);

  //waiting to avoid too fast reading of OK button from previous dialog
  wait();
  wait();

  uView.setCursor(0, 40);
  uView.print(timerSeconds);
  uView.display();

  while (digitalRead(OK) == HIGH) { //run the enter seconds dialog until the OK button is pressed

    enterNewTimerSecondsDialog();

    if (digitalRead(UP) == HIGH && waitToChange == false) {

      waitToChange = true;

      if (timerSeconds >= 59) {
        timerSeconds = 0;
      } else {
        timerSeconds += 1;
      }


      uView.clear(PAGE);
      enterNewTimerSecondsDialog();
      uView.setCursor(0, 40);
      uView.print(timerSeconds);
      uView.display();

      wait(); //waiting to avoid too fast changes in value

      waitToChange = false;

    } else if (digitalRead(DOWN) == HIGH && waitToChange == false) {

      waitToChange = true;

      if (timerSeconds <= 0) {
        timerSeconds = 59;
      } else {
        timerSeconds -= 1;
      }

      uView.clear(PAGE);
      enterNewTimerSecondsDialog();
      uView.setCursor(0, 40);
      uView.print(timerSeconds);
      uView.display();

      wait(); //waiting to avoid too fast changes in value
      waitToChange = false;
    }

  }

  setSoundOff();

  timerTotalInSeconds += timerSeconds;
  timerID = timer.setTimer(1000, doCountDown, timerTotalInSeconds + 1);
  timer.enable(timerID);
  timerIsOn = true;
}

void enterNewTimerSecondsDialog() {
  uView.setCursor(0, 0);
  uView.print("Enter");
  uView.setCursor(0, 12);
  uView.print("seconds:");
  uView.display();
}

void setSoundOff() {

  boolean waitToChange = false;

  uView.clear(PAGE);

  enterSoundOffDialog();
  uView.setCursor(0, 40);
  uView.print("No");
  uView.display();

  //waiting to avoid too fast reading of OK button from previous dialog
  wait();
  wait();

  while (digitalRead(OK) == HIGH) { //run the setSoundOff dialog until the OK button is pressed

    if (digitalRead(UP) == HIGH && waitToChange == false) {

      waitToChange = true;

      soundOff = true;

      enterSoundOffDialog();
      uView.setCursor(0, 40);
      uView.print("Yes");
      uView.display();

      wait();

      waitToChange = false;

    } else if (digitalRead(DOWN) == HIGH && waitToChange == false) {

      waitToChange = true;

      soundOff = false;

      enterSoundOffDialog();
      uView.setCursor(0, 40);
      uView.print("No");
      uView.display();

      wait();

      waitToChange = false;

    }

  }

}

void enterSoundOffDialog() {

  uView.clear(PAGE);
  uView.setCursor(0, 0);
  uView.print("Sound");
  uView.setCursor(0, 12);
  uView.print("off?");
  uView.display();

}

void setTemperature() {

  if (noInterrupt == false) {

    boolean waitToChange = false; //boolean for prevention of value changes while waiting

    uView.clear(PAGE);

    if (goalTemp == 0) { //if no previous goal temp has been set
      newGoalTemp = 60; //default goal value
    } else {
      newGoalTemp = goalTemp;
    }

    while (digitalRead(OK) == HIGH) { //run the menu until the OK button is pressed

      enterNewGoalDialog();

      if (digitalRead(UP) == HIGH && waitToChange == false) {
        waitToChange = true;
        if (newGoalTemp >= 99) {
          newGoalTemp = 99;
        } else {
          newGoalTemp += 1;
        }
        uView.setCursor(0, 40);
        uView.print(newGoalTemp);
        wait(); //waiting to avoid too fast changes in value
        waitToChange = false;

      } else if (digitalRead(DOWN) == HIGH && waitToChange == false) {
        waitToChange = true;
        if (newGoalTemp <= 0) {
          newGoalTemp = 0;
        } else {
          newGoalTemp -= 1;
        }
        uView.setCursor(0, 40);
        uView.print(newGoalTemp);
        wait(); //waiting to avoid too fast changes in value
        waitToChange = false;
        //the following two rows are to avoid "ghost digit" if the number of digits are changed, i.e. going from 10 to 9 would otherwise show as 90
        uView.clear(PAGE); //clearing the screen
        enterNewGoalDialog(); //showing the input dialog again
      }
    }

    goalTemp = newGoalTemp; //accepting the new value as actual goal temperature
    tempReached = false; //reinitiating the alarm

    setSoundOff();

    uView.clear(PAGE);
    showTempScreen();
    showTempScreenOnce = false;
  }

}

void showTempScreen() {

  if (!tempReached) { //ignore drawing heat waves if goal temperature has been met

    drawHeatWaves();

  } else { //display "Enjoy!" if goal temperature was met

   uView.setCursor(4,0);
   uView.print("T's ready!");
   uView.display();
    
  }




  drawTeaCup();

  if(!tempReached){

  uView.setCursor(10, 40);
  uView.print("Goal:");
  uView.setCursor(42, 40);
  uView.print(goalTemp);
  uView.display();
    
  }
  
}

void enterNewGoalDialog() {
  uView.invert(false);
  uView.clear(PAGE);
  uView.setCursor(0, 0);
  uView.print("Enter new");
  uView.setCursor(0, 12);
  uView.print("goal temp:");
  uView.setCursor(0, 40);
  uView.print(newGoalTemp);
  uView.display();
}

void doCountDown() {

  if (timerSeconds < 10) {
    uView.clear(PAGE);
    uView.setFontType(3);
    uView.setCursor(0, 0);
    uView.print(timerMinutes);
    uView.setCursor(13, 0);
    uView.print(":");
    uView.setCursor(39, 0);
    uView.print(timerSeconds);
    uView.setCursor(26, 0);
    uView.print("0");

    if (soundOff) {
      drawNoSoundIconTimer();
    }
    uView.display();


  } else {

    uView.clear(PAGE);
    uView.setFontType(3);
    uView.setCursor(0, 0);
    uView.print(timerMinutes);
    uView.setCursor(10, 0);
    uView.print(":");
    uView.setCursor(20, 0);
    uView.print(timerSeconds);

    if (soundOff) {
      drawNoSoundIconTimer();
    }

    uView.display();

  }

  if (timerMinutes >= 0) {

    if (timerSeconds <= 0) {
      timerMinutes -= 1;
      timerSeconds = 59;
    } else {
      timerSeconds -= 1;
    }
  }


  if (timerTotalInSeconds == 0) {

    uView.clear(PAGE);
    uView.setFontType(1);
    uView.setCursor(17, 0);
    uView.print("Time");
    uView.setCursor(25, 15);
    uView.print("is");
    uView.setCursor(23, 30);
    uView.print("up!");
    uView.display();
    uView.setFontType(0);

    while (digitalRead(OK) == HIGH) {

      //  updateBouncers();

      //sound the buzzer and wait for OK button press

      if (soundOff == false) {
        digitalWrite(snd, HIGH);
      }
      wait();
      if (soundOff == false) {
        digitalWrite(snd, LOW);
      }
      wait();
      uView.invert(true);
      if (soundOff == false) {
        digitalWrite(snd, HIGH);
      }
      wait();
      if (soundOff == false) {
        digitalWrite(snd, LOW);
      }
      wait();
      uView.invert(false);

    }

    timerIsOn = false;
    noInterrupt = false;
    soundOff = false;
  }

  timerTotalInSeconds -= 1;

}


float getTemp() {
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
    //no more sensors on chain, reset search
    ds.reset_search();
    return -10;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
    return -10;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28)
  {
    return -10;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44); // start conversion

  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++)   // we need 9 bytes
  {
    data[i] = ds.read();
  }

  ds.reset_search();

  byte MSB = data[1]; //most significant bit
  byte LSB = data[0]; //least significant bit

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  return TemperatureSum;

}

void wait() {   //method to simulate delay() outside of loop()
  for (int i = 0; i < waitingTime; i++) {
    while (delayCounter < waitingTime) {
      delayCounter++;
    }
    delayCounter = 0;
  }
}

void drawNoSoundIconTimer() { //TODO: make more efficient (use lineV, lineH etc.)

  uView.pixel(56, 38);
  uView.pixel(57, 38);
  uView.pixel(58, 38);
  uView.pixel(55, 39);
  uView.pixel(58, 39);
  uView.pixel(52, 40);
  uView.pixel(53, 40);
  uView.pixel(54, 40);
  uView.pixel(58, 40);
  uView.pixel(52, 41);
  uView.pixel(54, 41);
  uView.pixel(58, 41);
  uView.pixel(60, 41);
  uView.pixel(63, 41);
  uView.pixel(52, 42);
  uView.pixel(54, 42);
  uView.pixel(58, 42);
  uView.pixel(61, 42);
  uView.pixel(62, 42);
  uView.pixel(52, 43);
  uView.pixel(54, 43);
  uView.pixel(58, 43);
  uView.pixel(61, 43);
  uView.pixel(62, 43);
  uView.pixel(52, 44);
  uView.pixel(54, 44);
  uView.pixel(58, 44);
  uView.pixel(60, 44);
  uView.pixel(63, 44);
  uView.pixel(52, 45);
  uView.pixel(53, 45);
  uView.pixel(54, 45);
  uView.pixel(58, 45);
  uView.pixel(55, 46);
  uView.pixel(58, 46);
  uView.pixel(56, 47);
  uView.pixel(57, 47);
  uView.pixel(58, 47);
  uView.display();

}

void drawNoSoundIconTemperature() {

  uView.lineH(56, 0, 3);
  uView.pixel(55, 1);
  uView.lineV(58, 1, 8);
  uView.lineH(52, 2, 3);
  uView.lineV(52, 3, 4);
  uView.lineV(54, 3, 4);
  uView.lineH(52, 7, 3);
  uView.pixel(55, 8);
  uView.lineH(56, 9, 3);
  uView.rect(61, 4, 2, 2);
  uView.pixel(60, 3);
  uView.pixel(63, 3);
  uView.pixel(60, 6);
  uView.pixel(63, 6);
  uView.display();

}

void animateHeatWaves() {

  if (!heatWavesFlipped) {
    drawHeatWaves();
    uView.display();
    heatWavesFlipped = true;
  } else {
    drawHeatWavesFlipped();
    uView.display();
    heatWavesFlipped = false;
  }

}

void drawHeatWaves() {

  //draw black rectangle to erase any previously drawn waves

  uView.setColor(0); //set color to black
  uView.rectFill(13, 0, 36, 14);
  uView.setColor(1); //set color back to white

  //draw heat waves

  //left wave
  uView.lineV(22, 6, 5);
  uView.pixel(23, 0);
  uView.lineV(23, 4, 8);
  uView.lineV(24, 0, 7);
  uView.lineV(24, 10, 2);
  uView.lineV(25, 0, 6);

  //middle wave
  uView.rectFill(31, 0, 3, 5);
  uView.rectFill(30, 5, 3, 2);
  uView.rectFill(29, 6, 3, 7);
  uView.lineH(30, 13, 2);

  //right wave
  uView.rectFill(38, 0, 2, 7);
  uView.rectFill(37, 5, 2, 7);
  uView.lineV(40, 1, 4);
  uView.lineV(36, 7, 3);



}

void drawHeatWavesFlipped() { //horizontally flipped heat waves used for animation

  //draw black rectangle to erase any previously drawn waves

  uView.setColor(0);
  uView.rectFill(13, 0, 36, 14);
  uView.setColor(1);

  //draw flipped heat waves

  //left wave
  uView.rectFill(22, 0, 2, 6);
  uView.rectFill(23, 4, 2, 3);
  uView.rectFill(24, 6, 2, 5);
  uView.rectFill(23, 10, 2, 2);
  uView.pixel(24, 0);

  //middle wave
  uView.rectFill(28, 0, 3, 5);
  uView.rectFill(29, 5, 3, 2);
  uView.rectFill(30, 6, 3, 7);
  uView.lineH(30, 13, 2);

  //right wave
  uView.lineV(35, 1, 4);
  uView.rectFill(36, 0, 2, 7);
  uView.rectFill(37, 5, 2, 7);
  uView.lineV(39, 7, 3);
}

void drawTeaCup() {

  //cup and saucer
  uView.rectFill(15, 15, 32, 12);
  uView.lineV(14, 16, 9);
  uView.lineV(47, 15, 4);
  uView.rectFill(48, 16, 7, 2);
  uView.rectFill(54, 18, 3, 7);
  uView.rectFill(53, 17, 3, 2);
  uView.rectFill(53, 25, 3, 2);
  uView.rectFill(47, 26, 7, 2);
  uView.pixel(47, 25);
  uView.rectFill(20, 27, 22, 11);
  uView.rectFill(16, 27, 30, 2);
  uView.rectFill(18, 29, 26, 2);
  uView.lineH(17, 29, 28);
  uView.lineH(19, 31, 24);
  uView.rectFill(10, 34, 43, 2);
  uView.lineH(11, 36, 41);
  uView.lineH(12, 37, 39);

  uView.display();
}


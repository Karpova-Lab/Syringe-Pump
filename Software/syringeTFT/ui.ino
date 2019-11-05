void UI(void (*fxn)(uint16_t), uint16_t buttonPos){
    if (! (buttons & 1<<buttonPos)) {
        stepper.enableOutputs();
        tft.fillScreen(ST77XX_BLACK);
        (*fxn)(ST77XX_WHITE);
        uint8_t count = 0;
        if (buttonPos==B_BTN){
            stepper.setAcceleration(850); // 2000mm/s^2
            for(byte i=0; i<10;i++){
                driver.mres(5); 
                delay(5);
            };
        }
        else{
            stepper.setAcceleration(5000); // 2000mm/s^2
            for(byte i=0; i<10;i++){
                driver.mres(4); 
                delay(5);
            }
        }
        while(!(buttons & 1<<buttonPos)){
            stepper.run();
            switch (buttonPos){
            case UP:
                if((PINF & (1<<4) ) && (PIND & (1<<6))){
                    moveDirection(INFUSE);
                }
                else{
                    limitReached(INFUSE);
                    buttons = ss.readButtons();
                }
                break;
            case DOWN:
                if ((PINF & (1<<4) ) && (PIND & (1<<6))){
                    moveDirection(RETRACT);
                }
                else{
                    limitReached(RETRACT);
                    buttons = ss.readButtons();
                }
                break;
            case B_BTN: 
                retract();
                buttons = ss.readButtons();
                break;
            case A_BTN:
                delay(1000);
                ongoingPosition = 0;
                buttons = ss.readButtons();
                break;
            case CENTER:
                uint32_t holdStarted = millis();
                while( ((millis()-holdStarted) < 900) & (!(buttons & 1<<buttonPos)) ){
                    buttons = ss.readButtons();
                }
                if( !(buttons & 1<<buttonPos) ){
                    softDirection = !softDirection;
                    EEPROM.update(DIRECTION_ADDRESS, softDirection);
                    stepper.setPinsInverted(softDirection, false, true);
                    successfulFlip(ST77XX_WHITE);
                }
                break;
            }
            if (count++ == 254){
                count = 0;
                buttons = ss.readButtons();
            }
        }
    ongoingPosition += long(stepper.currentPosition());
    stepper.setCurrentPosition(0);    
    showMenu();
  }
}

void serialUI(){
  if (Serial1.available()){
    char msg = Serial1.read();
    if (msg=='I' | msg == 'C' | msg == 'R' | msg == 'Z'){
        tft.fillScreen(ST77XX_BLACK);
        if (msg=='I'){ 
            uint32_t volume = parseData();
            dispenseVolume(volume);
        }
        else if (msg=='C'){
            connectedMessage();
        }
        else if (msg == 'R'){
            retracting(ST77XX_WHITE);
            retract();
        }
        else if (msg == 'Z'){
            resetting(ST77XX_WHITE);
            delay(1000);
            ongoingPosition = 0;
        }
        ongoingPosition += long(stepper.currentPosition());
        stepper.setCurrentPosition(0);
        showMenu();
    }    
  }
}

void showMenu(){
    tft.fillScreen(ST77XX_BLACK);
    reset(ST77XX_WHITE);  
    showDispensed(ST77XX_GREEN);
}

void reset(uint16_t color){
    tft.setTextColor(color);
    tft.setTextSize(1);
    tft.setCursor(129,11);
    tft.print("Reset");
    tft.setCursor(123,62);
    tft.print("Refill");
}

void retracting(uint16_t color){
    tft.setTextColor(color);
    tft.setTextSize(3);
    tft.setCursor(30 , 30);
    tft.print("Refill");
}

void resetting(uint16_t color){
    tft.setTextColor(color);
    tft.setTextSize(3);
    tft.setCursor(40 , 17);
    tft.print("Reset");
    tft.setCursor(30 , 43);
    tft.print("Volume");
}

void pushing(uint16_t color){
    tft.setRotation(3);
    uint16_t centerX = 80; 
    uint16_t bottomSide = 60;
    uint16_t arrowWidth = 20;
    uint16_t arrowHeight = 40;
    tft.fillTriangle(centerX - arrowWidth, bottomSide, centerX, bottomSide - arrowHeight, centerX + arrowWidth,bottomSide, color);
}

void pulling(uint16_t color){
    uint16_t centerX = 80; 
    uint16_t topSide = 20;
    uint16_t arrowWidth = 20;
    uint16_t arrowHeight = 40;
    tft.fillTriangle(centerX - arrowWidth, topSide, centerX, topSide + arrowHeight, centerX + arrowWidth,topSide, color);
}

void flipDirection(uint16_t color){
    tft.setTextColor(color);
    tft.setTextSize(2);
    tft.setCursor(60 , 20);
    tft.print("Hold");
    tft.setCursor(20 , 36);
    tft.print("to reverse");
}

void successfulFlip(uint16_t color){
    tft.fillScreen(ST77XX_WHITE);
    delay(900);
}

void showDispensed(uint16_t color){
    tft.setTextColor(color);
    tft.setTextSize(3);
    tft.setCursor(0,30);
    tft.print(ongoingPosition*resolution/1000);  
    tft.setCursor(tft.getCursorX(),37);
    tft.setTextSize(2);
    tft.print(" mL");
}

void limitMessage(uint16_t color){
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(color);
    tft.setTextSize(3);
    tft.setCursor(40, 30);
    tft.print("Limit");
}

void connectedMessage(){
    tft.fillScreen(ST77XX_BLACK);;
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(0 , 0);
    tft.print("pyControl");
    delay(1500);
    ongoingPosition = 0;
    tft.fillScreen(ST77XX_BLACK);
    showMenu();
}
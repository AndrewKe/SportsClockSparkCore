/*
* Andrew Ke
* Sports Clock
*/

// subdirectory is required for Spark WEB IDE
#include "ledmatrix-max7219-max7221.h"
LEDMatrix *led;

int fontWidth = 5, space = 0;
int speakerPin = A5;
String mode = "scores";

void setup() {
    // setup pins and library
    // 4 display per row, 1 display per column
    // optional pin settings - default: CLK = A0, CS = A1, D_OUT = A2
    // (pin settings is independent on HW SPI)
    led = new LEDMatrix(4, 1, A0, A1, A2);

    // > add every matrix in the order in which they have been connected <
    // the first matrix in a row, the first matrix in a column
    // vertical orientation (-90°) and no mirroring - last three args optional
    led->addMatrix(0, 0, 180, true, true);
    led->addMatrix(1, 0, 180, true, true);
    led->addMatrix(2, 0, 180, true, true);
    led->addMatrix(3, 0, 180, true, true);

    Particle.function("setScore", setScore);
    Particle.function("setMode", setMode);
    Particle.function("setColor", setColor);
}

int nextScoreTime = 0;
int nextPingTime = 0;

void loop() {
    if (mode == "scores")
    {
        space = 0;
        if (nextScoreTime < millis()) {
            Particle.publish("score");
            Serial.println("asked for score!");
            nextScoreTime = millis() + 1000;
        }
        
        if (nextPingTime < millis())
        {
            Serial.println("Pinging server");
            Particle.publish("ping"); // connected to webhook
            Particle.syncTime();
            nextPingTime = millis() + 60*1000;
        }
    }else if (mode == "time")
    {
        space = 1;
        drawTime();
    }

}

void buzz(int duration)
{
    tone(speakerPin, 261, duration);
}
int setMode(String m)
{
    if (m == "DFU")
    {
        System.dfu();
        return 1;
    }
    clearScreen();
    mode = m;
    return 1;
}

int setColor(String hex)
{
    long number = strtoll( &hex[1], NULL, 16);

    // Split them up into r, g, b values
    long r = number >> 16;
    long g = number >> 8 & 0xFF;
    long b = number & 0xFF;

    Serial.printlnf("Setting color to %d %d %d", r, g, b);
    return 1;
}
/*
*   LED Matrix Score Drawing
*/

void drawText(String s, int x)
{
    int y = 0;
    for(int i = 0; i < s.length(); i++) {
        // Adafruit_GFX method
        led->drawChar(x + i*(fontWidth+space), y, s[i], true, false, 1);
    }
}

// draws inning
void drawInning(bool up, int x) {
    int y = 3;
    if (up){
        led->drawLine(x, 3, x, 0,  true);
    }
    else{
        led->drawLine(x, 3, x, 7, true);
    }
}

int home_score, away_score, inning, outs;
bool top;

int setScore(String input)
{
    Serial.println("Got score!");
    if (getValue(input, ',' , 0) > home_score)
    {
        // buzz good sound
    }
    if (getValue(input, ',' , 1) > away_score)
    {
        // buzz bad sound
    }
    home_score = getValue(input, ',' , 0);
    away_score = getValue(input, ',', 1);
    inning = getValue(input, ',', 2);
    top = getValue(input, ',', 3);
    outs = getValue(input, ',', 4);
    drawScore();
    return 1;
}

void drawScore()
{
    Serial.printf("%d - %d %s %d %d outs \n", home_score, away_score, top ? "top" : "bottom", inning, outs);
    String score1 = String(home_score);
    String score2 = String(away_score);
    String inningStr = String(inning);

    int lengthOfScore = score1.length()*5 + score2.length()*5 + 3;
    int firstOffset = (score1.length())*5 + 1;
    int inningOffset = 30-inningStr.length()*5-2;
    int scoreOffset = (inningOffset-lengthOfScore)/2;

    drawText(score1, scoreOffset);

    led->bitmap->setPixel(firstOffset+scoreOffset, 3, true);
    drawText(score2, firstOffset+scoreOffset + 2);

    if (inningStr.length()>1 && (score1.length()+score2.length())>2){
        drawInning(top, inningOffset+1);
    }
    else {
        drawInning(top, inningOffset);
    }
    drawText(inningStr, inningOffset+2);

    drawOuts(outs);

    led->flush();
}

void clearScreen()
{
    led->fillScreen(false);
    led->flush();
}

void drawOuts(int outs)
{
    int x = 31;
    if (outs > 0){
        led->bitmap->setPixel(x, 0, true);
    }
    if (outs > 1) {
        led->bitmap->setPixel(x, 3, true);
    }
    if (outs > 2){
        led->bitmap->setPixel(x, 7, true);
    }
}

/*
*   Time
*/

void drawTime(){
    Time.zone(-8);
    String t = String(Time.hourFormat12()) + ":" + ((Time.minute()<10) ? "0" : "") + String(Time.minute());
    Serial.println(t);
    int offset = (32-(t.length() *5) - (t.length()-1) )/2;
    drawText(t, offset);
    led->flush();
}

/*
*   Utility methods
*/

// String utility function from http://stackoverflow.com/questions/9072320/split-string-into-string-array
int getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length()-1;

    for(int i=0; i<=maxIndex && found<=index; i++){
        if(data.charAt(i)==separator || i==maxIndex){
            found++;
            strIndex[0] = strIndex[1]+1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }

    return (found>index ? data.substring(strIndex[0], strIndex[1]) : "0").toInt();
}

#include "FastLED.h"
#include "Enemy.h"
#include "Spawner.h"
#include "Lava.h"

//Treba testirati
#define LEVEL_NUMBER 10
#define ATTACK_SIZE 70
#define ATTACK_DURATION 500


const int leftPin = 2;
const int rightPin = 3;
const int attackPin = 4;
int const enemyCount = 10;
int const spawnCount = 2;
int const lavaCount = 4;

long previousTime = 0;
int levelNumber = 0;
long lastInputTime = 0;
char* state;
int playerPosition;
bool playerAlive;
long attackStart = 0;
bool attacking = 0;

//Prepreke
Enemy enemyPool[10] = {Enemy(), Enemy(), Enemy(), Enemy(), Enemy(), Enemy(), Enemy(), Enemy(), Enemy(), Enemy()};
Spawner spawnPool[2] = {Spawner(), Spawner()};
Lava lavaPool[4] = {Lava(), Lava(), Lava(), Lava()};

CRGB leds[120];

void setup() {
    Serial.begin(9600);
    while (!Serial);

    //Podesavanje dugmica za pomeranje i napadanje kao ulaza
    pinMode(leftPin, INPUT);
    pinMode(rightPin, INPUT);
    pinMode(attackPin, INPUT);

    //Podesavanje LED strip-a
    FastLED.addLeds<WS2812, ledPin, GRB>(leds, 120);
    FastLED.setBrightness(150);
    FastLED.setDither(1);
    
    loadLevel();
}

void loop() {
    long time = millis();
    int brightness = 0;
    //Refresh 60 puta po sekundi
    if (time - previousTime >= 16){
        previousTime = time;
        if(state == "PLAY"){
            if(attacking && attackStart + ATTACK_DURATION < time)
                attacking = 0; //Proverava da li je napad gotov          
            if(!attacking && digitalRead(attackPin) == HIGH){ //Zapocinje napad ako je pritisnuto dugme za napad
                attackStart = time;
                attacking = 1;
            }
            if(!attacking){
                int moveAmount;
                //Proverava u koju stranu treba da se krece u zavisnosti od toga sta je kliknuto
                if(digitalRead(rightPin) == HIGH && digitalRead(leftPin) != HIGH)
                    moveAmount = 10;
                else if(digitalRead(leftPin) == HIGH && digitalRead(rightPin) != HIGH)
                    moveAmount = -10;
                else //Ako nijedno dugme nije kliknuto ili su kliknuti i lefi i right ne krece se
                    moveAmount = 0;                
                playerPosition += moveAmount;
                //Ne dopusta igracu da napusti opseg level-a
                if(playerPosition < 0)
                    playerPosition = 0;
                //Igrac pobedjuje level ako dodje do kraja
                if(playerPosition >= 1000)
                    levelComplete();
            }
            if(inLava(playerPosition))
                die();
            //Refreshuje ceo LED strip i prikaze sve na novim lokacijama/u novim stanjima
            FastLED.clear();
            tickSpawners();
            tickLava();
            tickEnemies();
            drawPlayer();
            drawAttack();
            drawExit();
        }
        else if(state == "DEAD"){
            FastLED.clear();
            gameOver();    
        }
        else if(state == "WIN"){
            FastLED.clear();
            nextLevel();
        }
        FastLED.show();
    }
}

//Pronalazi koje LEDove treba ukljuciti/iskljuciti/promeniti
int getLED(int position){
    return constrain((int)map(position, 0, 1000, 0, 119), 0, 119);
}

//Popuniti level-ima kad ih smislimo
void loadLevel(){
    //...
    state = "PLAY";
}

//Prelazi na sledeci level
void nextLevel(){
    levelNumber++;
    if(levelNumber > LEVEL_NUMBER)
        levelNumber = 1; //Ako su svi level-i predjeni vraca se na prvi
    loadLevel();
}

void levelComplete(){
    state = "WIN";
}

//Vraca se na pocetni level
void gameOver(){
    levelNumber = 1;
    loadLevel();
}

//Stvori enemy-ja koji se krece u pravcu direction brzinom speed
//Patrolira od position do position +/- patrolWidth
//Ako je patrolWidth == 0, samo se krece u pravcu direction
void spawnEnemy(int position, int direction, int speed, int patrolWidth){
    for(int i = 0; i < enemyCount; ++i)
        if(!enemyPool[i].Alive()){
            enemyPool[i].Spawn(position, direction, speed, patrolWidth);
            enemyPool[i].playerSide = position > playerPosition ? 1:-1;
            return;
        }
}

//Stvori lavu izmedju left i right koja je ukljucena onDuration ms i iskljucena offDuration ms
void spawnLava(int left, int right, int onDuration, int offDuration, int offset, char* state){
    for(int i = 0; i < lavaCount; ++i)
        if(!lavaPool[i].Alive()){
            lavaPool[i].Spawn(left, right, onDuration, offDuration, offset, state);
            return;
        }
}

//Proveri da li je pozicija unutar aktivne lave
bool inLava(int position){
    int i;
    Lava lavaPool_;
    for(i = 0; i < lavaCount; ++i){
        lavaPool_ = lavaPool[i];
        if(lavaPool_.Alive() && lavaPool_._state == "ON")
            if(lavaPool_._left < position && lavaPool_._right > position)
                return true;
    }
    return false;
}

void die(){
    playerAlive = 0;
    levelNumber = 0;
    state = "DEAD";
}

void drawPlayer(){
    leds[getLED(playerPosition)] = CRGB(0, 255, 0);
}

void drawExit(){
    leds[119] = CRGB(0, 0, 255);
}

//Kontrolise enemy-je
void tickEnemies(){
    for(int i = 0; i < enemyCount; --i){
        if(enemyPool[i].Alive()){
            enemyPool[i].Tick();
            if(attacking)
                if(enemyPool[i]._pos > playerPosition-(ATTACK_SIZE/2) && enemyPool[i]._pos < playerPosition+(ATTACK_SIZE/2)) //Proverava da li je enemy udaren
                   enemyPool[i].Kill();
            if(inLava(enemyPool[i]._pos)) //Proverava da li je enemy u lavi
                enemyPool[i].Kill();
            if(enemyPool[i].Alive())
                leds[getLED(enemyPool[i]._pos)] = CRGB(255, 0, 0); //Prikaze enemy-ja
            if((enemyPool[i].playerSide == 1 && enemyPool[i]._pos <= playerPosition) || (enemyPool[i].playerSide == -1 && enemyPool[i]._pos >= playerPosition)){ //Proveri da li se enemy sudario sa igracem
                die();
                return;
            }
        }
    }
}

//Kontrolise spawner-e
void tickSpawners(){
    long timeS = millis();
    for(int i = 0; i < spawnCount; ++i)
        if(spawnPool[i].Alive() && spawnPool[i]._activate < timeS)
            if(spawnPool[i]._lastSpawned + spawnPool[i]._rate < timeS || spawnPool[i]._lastSpawned == 0){ //Proverava da li treba da spawn-uje novog enemy-ja
                spawnEnemy(spawnPool[i]._pos, spawnPool[i]._dir, spawnPool[i]._sp, 0);
                spawnPool[i]._lastSpawned = timeS;
            }
}

//Kontrolise lavu
void tickLava(){
    int A, B, j, i, brightness, flicker;
    long timeL = millis();
    Lava lavaPool_;
    for(i = 0; i < lavaCount; ++i){
        flicker = random8(5);
        lavaPool_ = lavaPool[i];
        if(lavaPool_.Alive()){
            A = getLED(lavaPool_._left);
            B = getLED(lavaPool_._right);
            if(lavaPool_._state == "OFF"){
                if(lavaPool_._lastOn + lavaPool_._offtime < timeL){ //Ukljucuje lavu kad treba
                    lavaPool_._state = "ON";
                    lavaPool_._lastOn = timeL;
                }
                for(j = A; j <= B; ++j) //Osvetli sva polja lave
                    leds[j] = CRGB(3 + flicker, (3 + flicker)/1.5, 0); //flicker sluzi da lava treperi kako bi se lakse primetila
            }
            else if(lavaPool_._state == "ON"){
                if(lavaPool_._lastOn + lavaPool_._ontime < timeL){ //Iskljucuje lavu kad treba
                    lavaPool_._state = "OFF";
                    lavaPool_._lastOn = timeL;
                }
                for(j = A; j <= B; ++j) //Osvetli sva polja lave
                    leds[j] = CRGB(150 + flicker, 100 + flicker, 0); //flicker sluzi da lava treperi kako bi se lakse primetila
            }
        }
        lavaPool[i] = lavaPool_;
    }
}

//Animacija za napad
void drawAttack(){
    if(!attacking)
        return;
    int n = map(millis() - attackStart, 0, ATTACK_DURATION, 100, 5); //Da bi se dobio fade efekat
    for(int i = getLED(playerPosition - (ATTACK_SIZE/2)) + 1; i <= getLED(playerPosition + (ATTACK_SIZE/2)) - 1; ++i)
        leds[i] = CRGB(0, 0, n);
    if(n > 90) {
        n = 255;
        leds[getLED(playerPosition)] = CRGB(255, 255, 255);
    }
    else{
        n = 0;
        leds[getLED(playerPosition)] = CRGB(0, 255, 0);
    }
    leds[getLED(playerPosition - (ATTACK_SIZE/2))] = CRGB(n, n, 255);
    leds[getLED(playerPosition + (ATTACK_SIZE/2))] = CRGB(n, n, 255);
}
#include "Arduino.h"

class Spawner
{
  public:
    void Spawn(int position, int rate, int speed, int direction, long activate);
    void Kill();
    int Alive();
    int _position;
    int _rate;
    int _speed;
    int _direction;
    long _lastSpawned;
    long _activate;
  private:
    int _alive;
};

void Spawner::Spawn(int position, int rate, int speed, int direction, long activate){
    _position = position;
    _rate = rate;
    _speed = speed;
    _direction = direction;
    _activate = millis() + activate;
    _alive = 1;
}

void Spawner::Kill(){
    _alive = 0;
    _lastSpawned = 0;
}

int Spawner::Alive(){
    return _alive;
}
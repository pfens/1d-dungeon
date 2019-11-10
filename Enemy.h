#include "Arduino.h"

class Enemy
{
  public:
    void Spawn(int position, int direction, int speed, int patrolWidth);
    void Tick();
    void Kill();
    bool Alive();
    int _position;
    int _patrolWidth;
    int playerSide;
  private:
    int _direction;
    int _speed;
    int _alive;
    int _origin;
};

void Enemy::Spawn(int position, int direction, int speed, int patrolWidth){
    _position = position;
    _direction = direction;
    _patrolWidth = patrolWidth;
    _origin = position;
    _speed = speed;
    _alive = 1;
}

void Enemy::Tick(){
    if(_alive){
        if(_patrolWidth > 0)
            _position = _origin + (sin((millis()/3000.0)*_speed)*_patrolWidth);
        else{
            if(_direction == 0)
                _position -= _speed;
            else
                _position += _speed;
            if(_position > 1000)
                Kill();
            if(_position <= 0)
                Kill();
        }
    }
}

bool Enemy::Alive(){
    return _alive;
}

void Enemy::Kill(){
    _alive = 0;
}
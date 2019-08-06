#pragma once

// ------------------------

// �÷��̾� �ʱ� ��ġ
#define INITPOSITION_X 62
#define INITPOSITION_Z 378

#define STATE_GROUND	 1
#define STATE_JUMPING 	2
#define STATE_ONOBJECTS 4
#define STATE_FALLING   5
#define STATE_STUN		6


// -------------------------
// ����
#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE 4000

// ����
#define NUM_OF_PLAYER 2

// 
#define CS_UP    1
#define CS_DOWN  2
#define CS_LEFT  3
#define CS_RIGHT    4

#define SC_POS           1
#define SC_PUT_PLAYER    2
#define SC_REMOVE_PLAYER 3
#define SC_CHAT			 4


//ĳ���� �ִϸ��̼�
#define HERO_ANIMATION_IDLE 	0
#define HERO_ANIMATION_WALK	1
#define HERO_ANIMATION_JUMP	2


//���� ����
#define MONTYPE_SNAKE		0
#define MONTYPE_BULKHOUSE	1
#define MONTYPE_TOXANIUM	2

// ���� ����
#define SOUND_COUNT  12

// ----------------------------------
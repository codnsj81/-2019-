//***************************************************************************************
// GameTimer.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#ifndef GAMETIMER_H
#define GAMETIMER_H

class GameTimer
{
public:
	GameTimer();

	float TotalTime()const; // in seconds
	float DeltaTime()const; // in seconds

	void Reset(); // �޽��� ���� ����
	void Start(); //Ÿ�̸� ����, �簳
	void Stop();  //Ÿ�̸� ����
	void Tick();  //�� ������ ȣ��

private:
	double mSecondsPerCount;
	double mDeltaTime;

	__int64 mBaseTime; // reset�� ȣ�� �� ��, ���� �ð����� �ʱ�ȭ
	__int64 mPausedTime; // Ÿ�̸Ӱ� �Ͻ������� ���� ����ؼ� ����,
						// ��ȿ�� ��ü �ð� : �����ð� - ������ �Ͻ����� �ð�
	__int64 mStopTime; //Ÿ�̸Ӱ� ������ ������ �ð�
	__int64 mPrevTime;
	__int64 mCurrTime;

	bool mStopped;
};

#endif // GAMETIMER_H
#ifndef __HPTIMER_H_
#define __HPTIMER_H_
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NHPTimer
{
	typedef int64 STime;
	// Compatibility entry point; the monotonic clock needs no calibration.
	void UpdateHPTimerFrequency();
	double GetSeconds( const STime &a );
	// �������� ������� �����
	void GetTime( STime *pTime );
	// �������� �����, ��������� � �������, ����������� � *pTime, ��� ���� � *pTime ����� �������� ������� �����
	double GetTimePassed( STime *pTime );
	// �������� ������� ����������
	double GetClockRate();
};
////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
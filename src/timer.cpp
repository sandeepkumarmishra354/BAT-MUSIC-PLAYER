#include "timer.h"

Timer::Timer(QWidget *parent):QDialog(parent)
{
    countDown = new QTimer;
    countDown->setSingleShot(true);
    connect(countDown, SIGNAL(timeout()), parent, SLOT(timeOut()));
    timerSpinBox = new QSpinBox;
    timerSpinBox->setRange(0, 5*60);//max 5*60 minutes(5 hour)
    minLbl = new QLabel("Min.");
    okBtn = new QPushButton("Start");
    connect(okBtn, SIGNAL(clicked()), this, SLOT(countDownStart()));
    hLayout = new QHBoxLayout;
    hLayout->addWidget(timerSpinBox);
    hLayout->addWidget(minLbl);
    hLayout->addWidget(okBtn);
    setLayout(hLayout);
    setMaximumHeight(300);
    setMaximumWidth(300);
    setWindowTitle("Set Timer");
}
void Timer::countDownStart()
{
    int minValue = timerSpinBox->value();
    if(minValue <= 0)
        countDown->stop();
    else
    {
        countDown->setInterval(minValue*1000*60);// in minute
        countDown->start();
    }
    timerSpinBox->setValue(0);
    close();
}
Timer::~Timer()
{
    delete hLayout;
    delete countDown;
}

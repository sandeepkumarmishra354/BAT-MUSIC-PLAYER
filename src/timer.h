#ifndef TIMER_H
#define TIMER_H

#include <QDialog>
#include <QWidget>
#include <QTimer>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

class Timer : public QDialog
{
    Q_OBJECT

private:
    QTimer *countDown;
    QSpinBox *timerSpinBox;
    QLabel *minLbl;
    QPushButton *okBtn;
    QHBoxLayout *hLayout;
public:
    Timer(QWidget *parent = 0);
    ~Timer();
private slots:
    void countDownStart();
};

#endif // TIMER_H

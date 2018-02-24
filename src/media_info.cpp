#include <QCursor>
#include <QPropertyAnimation>
#include <iostream>
#include "media_info.h"

Media_info::Media_info(QWidget *parent):QDialog(parent)
{

    song_lbl = new QLabel("Name: ");
    duration_lbl = new QLabel("Duration: ");
    artist_lbl = new QLabel("Artist: ");
    path_lbl = new QLabel("Path: ");
    codec_lbl = new QLabel("Codec: ");
    bitrate_lbl = new QLabel("Bit-Rate: ");

    song_lbl->setStyleSheet("color:#fb8c00; font-weight:bold");
    duration_lbl->setStyleSheet("color:#fb8c00; font-weight:bold");
    artist_lbl->setStyleSheet("color:#fb8c00; font-weight:bold");
    path_lbl->setStyleSheet("color:#fb8c00; font-weight:bold");
    codec_lbl->setStyleSheet("color:#fb8c00; font-weight:bold");
    bitrate_lbl->setStyleSheet("color:#fb8c00; font-weight:bold");

    song_name = new QLabel;
    song_name->setStyleSheet("color:white");
    duration = new QLabel;
    duration->setStyleSheet("color:white");
    artist = new QLabel;
    artist->setStyleSheet("color:white");
    path = new QLabel;
    path->setStyleSheet("color:white");
    codec = new QLabel;
    codec->setStyleSheet("color:white");
    bitrate = new QLabel;
    bitrate->setStyleSheet("color:white");

    formLayout = new QFormLayout;
    formLayout->addRow(song_lbl, song_name);
    formLayout->addRow(artist_lbl, artist);
    formLayout->addRow(duration_lbl, duration);
    formLayout->addRow(path_lbl, path);
    formLayout->addRow(codec_lbl, codec);
    formLayout->addRow(bitrate_lbl, bitrate);

    ok_btn = new QPushButton("Ok");
    ok_btn->setCursor(QCursor(Qt::PointingHandCursor));
    ok_btn->setStyleSheet("QPushButton::hover{background-color:#8080ff;}");
    ok_btn->setMaximumWidth(200);
    connect(ok_btn, SIGNAL(clicked()), this, SLOT(close()));

    hLayout = new QHBoxLayout;
    hLayout->addWidget(ok_btn);

    vLayout = new QVBoxLayout;
    vLayout->addLayout(formLayout);
    vLayout->addLayout(hLayout);

    setLayout(vLayout);
    setMinimumHeight(400);
    setMaximumHeight(500);
    setMaximumWidth(600);
    setWindowTitle("Media Information");
    setStyleSheet("background-color:#303036");
}
void Media_info::currentMedia(const QMediaResource &media)
{
    QString codec_tmp;
    QString bitrate_tmp;
    QString song_name_tmp;
    QString path_tmp;
    int bit_rate;
    QString a_codec;

    bit_rate = media.audioBitRate();

    if(bit_rate == 0)
        bitrate_tmp = "NA";// Not Available
    else
        bitrate_tmp += QString::number(bit_rate);

    a_codec = media.audioCodec();

    if(a_codec.isNull())
        codec_tmp = "NA";// Not Available
    else
        codec_tmp = a_codec;

    song_name_tmp = media.url().fileName();
    path_tmp = media.url().path();

    song_name->setText(song_name_tmp);
    path->setText(path_tmp);
    codec->setText(codec_tmp);
    bitrate->setText(bitrate_tmp);
}
void Media_info::totalDuration(QLabel* t_dur)
{
    duration->setText(t_dur->text());
}
void Media_info::artistName(QString singer)
{
    if( singer.contains("www.", Qt::CaseInsensitive) ||
            singer.contains(".com", Qt::CaseInsensitive) ||
            singer.contains(".in", Qt::CaseInsensitive) ||
            singer.contains("download", Qt::CaseInsensitive) ||
            singer.isEmpty() )
    {
        singer = "NA"; // Not Available
    }
    artist->setText(singer);
}
int Media_info::exec()
{
    this->setWindowOpacity(0.0);
    QPropertyAnimation *animate = new QPropertyAnimation(this, "windowOpacity");
    animate->setDuration(3000);
    animate->setEasingCurve(QEasingCurve::OutBack);
    animate->setStartValue(0.0);
    animate->setEndValue(1.0);
    animate->start(QAbstractAnimation::DeleteWhenStopped);
    return QDialog::exec();
}
Media_info::~Media_info()
{
    delete vLayout;
    delete song_lbl;
    delete duration_lbl;
    delete artist_lbl;
    delete path_lbl;
    delete codec_lbl;
    delete bitrate_lbl;
    delete song_name;
    delete duration;
    delete artist;
    delete path;
    delete codec;
    delete bitrate;
}

#ifndef MEDIA_INFO_H
#define MEDIA_INFO_H

#include <QDialog>
#include <QLabel>
#include <QWidget>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMediaResource>
#include <QPushButton>

class Media_info:public QDialog
{
    Q_OBJECT

    private:

        QLabel *song_lbl, *path_lbl, *codec_lbl;
        QLabel *bitrate_lbl, *duration_lbl, *artist_lbl;
        QFormLayout *formLayout;
        QVBoxLayout *vLayout;
        QHBoxLayout *hLayout;
        QPushButton *ok_btn; //cancel_btn;

        QLabel *codec, *path;
        QLabel *bitrate, *song_name;
        QLabel *duration, *artist;

    public:

        Media_info(QWidget *parent = 0);
        ~Media_info();
        void currentMedia(const QMediaResource& media);
        void totalDuration(QLabel* t_dur);
        void artistName(QString);
        int exec();
};
#endif // MEDIA_INFO_H

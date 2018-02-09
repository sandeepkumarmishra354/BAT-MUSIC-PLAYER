#ifndef MUSIC_PLAYER_H
#define MUSIC_PLAYER_H
#define NO_CHANGE -1
#define YES true
#define NO false

#include <QMainWindow>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QMediaPlaylist>
#include <QMediaContent>
#include <QMediaResource>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QSlider>
#include <QProgressBar>
#include <QWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QAction>
#include <QPoint>
#include <QVector>
#include <QColor>
#include <QBrush>
#include <QList>
#include <QMap>

class Music_Player : public QMainWindow
{
    Q_OBJECT

    private:

        QMediaPlayer *song_player;
        QMediaPlaylist *main_playlist;
        QPushButton *add_folder_btn;
        QWidget *main_container, *toolbar_container;
        QTableWidget *song_table;
        QHBoxLayout *main_Hlayout, *mDetail_Hlayout, *mControl_Hlayout, *volControl_Hlayout;
        QVBoxLayout *Vlayout1, *mControl_Vlayout, *mDetail_Vlayout;
        QSlider *song_progress, *volume_progress;
        QStatusBar *status_bar;
        QToolBar *tool_bar;
        QMenu *player_menu, *about_menu, *playlist_menu;
        QMenu *playlist_sub_menu, *add_to_sub_menu;
        QAction *main_playlist_action;
        QMenu *context_menu;
        QAction *create_new_playlist, *add_to_queue;
        QAction *remove_song, *delete_song;
        QAction *about_qt, *about_bat_player;
        QAction *exit_action, *refresh_action;
        QLabel *TOTAL_DUR, *CURRENT_DUR, *currrent_play_lbl;
        QLabel *total_song_lbl, *current_playing_lbl;
        QPushButton *play_pause_btn, *next_btn, *prev_btn, *info_btn;
        QPushButton *repeat_btn, *shuffle_btn, *vol_icon_btn;

        QMap <QString, QAction*> playlist_action_container;
        QMap <QString, QAction*> addto_action_container;
        QMap <QString, QMediaPlaylist*> playlist_container;
        QVector <int> song_queue;
        bool first_queue = true;

        QStringList all_songs;
        QMediaResource media_resource;
        bool any_changes, first_tym = true;
        QTableWidgetItem *prev_item;
        QBrush fg_brush, bg_brush;
        QColor bg_color, txt_clr;
        bool repeat_all, repeat_current, no_repeat, refresh, from_refresh;
        bool force_quit, no_folder_selected, is_mute, is_shuffle;
        bool auto_changed, by_next, by_prev, already;
        QString pl_path, info;
        short cur_volume, total_songs;

        void create_action_and_control();
        void load_music();
        void display_songs();
        void show_all_songs();
        void set_style();
        void set_music_layout();
        void create_context_menu();
        void enable_control(bool);
        void LoadPlaylist(QString);
        void savePlaylistToFile(const QString& pl_name, const QString& s_name);
        void readPlaylistFromFile();
        void generate_media_info();

        struct settings
        {
            QString playlist_path;
            short volume = NO_CHANGE;
            short duration = NO_CHANGE;
            int row_no = NO_CHANGE;
        }all_settings;

        enum class REPEAT
        {
            CURRENT, ALL, NONE
        };

        void repeat(const REPEAT&);

   public:

        Music_Player(QWidget *parent = 0);
        ~Music_Player();
        void closeEvent(QCloseEvent *);

   private slots:

        void exit_slot();
        void play_or_pause();
        void next_track();
        void prev_track();
        void add_folder();
        void set_repeat();
        void set_shuffle();
        void refresh_library();
        void mute_unmute();
        void about_player();
        void NewPlaylist();
        void set_focus();
        void add_queue();
        void remove_();
        void delete_();
        void show_song_info();
        void set_volume(int);
        void forwarded(int);
        void total_dur(qint64);
        void current_dur(qint64);
        void set_song_name(QMediaContent);
        void song_clicked(QTableWidgetItem*);
        void show_context_menu(QPoint);
        void AddTo(QAction*);
        void OpenPlaylist(QAction*);

};
#endif // MUSIC_PLAYER_H
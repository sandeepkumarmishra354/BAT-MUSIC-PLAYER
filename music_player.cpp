#include "music_player.h"
#include <iostream>
#include <QFileDialog>
#include <QHeaderView>
#include <QDir>
#include <QtDebug>
#include <QScrollBar>
#include <QPalette>
#include <QCursor>
#include <QCloseEvent>
#include <QTextStream>
#include <QDataStream>
#include <QFile>
#include <QIODevice>
#include <QApplication>
#include <QMessageBox>
#include <QInputDialog>
#include <QFont>
#include <QObject>

Music_Player::Music_Player(QWidget *parent):QMainWindow(parent)
{
    bg_color.setNamedColor("#e65100");
    txt_clr.setNamedColor("white");

    player_menu = menuBar()->addMenu(tr("&BAT-PLAYER"));
    playlist_menu = menuBar()->addMenu(tr("&Playlist"));
    playlist_sub_menu = playlist_menu->addMenu("Your Playlist");
    connect(playlist_sub_menu, SIGNAL(triggered(QAction*)), this, SLOT(OpenPlaylist(QAction*)));
    about_menu = menuBar()->addMenu(tr("&About"));
    tool_bar = addToolBar(tr("tool bar"));
    status_bar = statusBar();
    tool_bar->setMovable(false);

    create_action_and_control();
    create_context_menu();

    CURRENT_DUR = new QLabel("0:0");
    TOTAL_DUR = new QLabel("0:0");

    volume_progress = new QSlider(Qt::Horizontal);
    volume_progress->setRange(0,100);
    volume_progress->setValue(100);
    volume_progress->setFixedWidth(100);
    volume_progress->setCursor(QCursor(Qt::PointingHandCursor));
    volume_progress->setToolTip("Volume");
    connect(volume_progress, SIGNAL(valueChanged(int)), this, SLOT(set_volume(int)));

    song_progress = new QSlider(Qt::Horizontal);
    song_progress->setCursor(QCursor(Qt::PointingHandCursor));
    connect(song_progress, SIGNAL(sliderMoved(int)), this, SLOT(forwarded(int)));

    main_container = new QWidget;
    toolbar_container = new QWidget;
    currrent_play_lbl = new QLabel("BAT-MUSIC-PLAYER");

    song_player = new QMediaPlayer;
    connect(song_player, SIGNAL(durationChanged(qint64)), this, SLOT(total_dur(qint64)));
    connect(song_player, SIGNAL(positionChanged(qint64)), this, SLOT(current_dur(qint64)));
    connect(song_player, SIGNAL(currentMediaChanged(QMediaContent)), this, SLOT(set_song_name(QMediaContent)));

    main_playlist = new QMediaPlaylist(song_player);

    song_table = new QTableWidget;
    song_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    song_table->verticalHeader()->setVisible(false);
    song_table->horizontalHeader()->setVisible(false);
    song_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    song_table->setShowGrid(false);
    song_table->setContextMenuPolicy(Qt::CustomContextMenu);
    song_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    song_table->setSelectionMode(QAbstractItemView::SingleSelection);
    //song_table->setSelectionMode(QAbstractItemView::MultiSelection);
    connect(song_table, SIGNAL(cellClicked(int,int)), this, SLOT(set_focus()));
    connect(song_table, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(song_clicked(QTableWidgetItem*)));
    connect(song_table, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(show_context_menu(QPoint)));

    total_song_lbl = new QLabel;
    current_playing_lbl = new QLabel;
    status_bar->addPermanentWidget(current_playing_lbl);
    status_bar->addPermanentWidget(total_song_lbl);

    // set flags initial state
    repeat_all = repeat_current = no_repeat = false;
    force_quit = false;
    refresh = false;
    is_mute = false;
    is_shuffle = false;
    any_changes = false;
    no_folder_selected = true;
    auto_changed = true;
    by_next = by_prev = false;
    already = false;
    from_refresh = false;
    Vlayout1 = nullptr;
    add_folder_btn = nullptr;

    set_music_layout();
    set_style();
    load_music();

    playlist_container.insert("Main Playlist", main_playlist);

    setWindowIcon(QIcon(":player/icons/Bat_player.ico"));
    setMinimumWidth(800);
    setMinimumHeight(650);
    setWindowTitle("BAT-MUSIC-PLAYER");
    showMaximized();
}

void Music_Player::create_context_menu()
{
    context_menu = new QMenu(this);
    create_new_playlist = new QAction("New Playlist");
    connect(create_new_playlist, SIGNAL(triggered()), this, SLOT(NewPlaylist()));
    context_menu->addAction(create_new_playlist);

    add_to_sub_menu = context_menu->addMenu("Add To");
    connect(add_to_sub_menu, SIGNAL(triggered(QAction*)), this, SLOT(AddTo(QAction*)));

    add_to_queue = new QAction("Add to Queue");
    context_menu->addAction(add_to_queue);
    connect(add_to_queue, SIGNAL(triggered()), this, SLOT(add_queue()));

    remove_song = new QAction("Remove");
    remove_song->setToolTip("Remove song from current playlist");
    connect(remove_song, SIGNAL(triggered()), this, SLOT(remove_()));
    context_menu->addAction(remove_song);

    delete_song = new QAction("Delete");
    delete_song->setToolTip("Delete song from disk storage");
    delete_song->setIcon(QIcon(":player/icons/Delete.png"));
    connect(delete_song, SIGNAL(triggered()), this, SLOT(delete_()));
    context_menu->addAction(delete_song);
}

void Music_Player::remove_()
{
    QList<QTableWidgetItem*> list;
    list = song_table->selectedItems();
    if(!list.isEmpty())
    {
        foreach(QTableWidgetItem *itm, list)
        {
            song_player->playlist()->removeMedia(itm->row());
            song_table->removeRow(itm->row());
            total_songs--;
            total_song_lbl->setText("Total Song "+QString::number(total_songs));
        }
    }
}

void Music_Player::delete_()
{
    QList<QTableWidgetItem*> list;
    list = song_table->selectedItems();
    if(!list.isEmpty())
    {
        foreach(QTableWidgetItem *itm, list)
        {
            QString file = pl_path + "/" + itm->text();
            song_player->playlist()->removeMedia(itm->row());
            song_table->removeRow(itm->row());
            QFile rm_file(file);
            rm_file.remove();
            total_songs--;
            total_song_lbl->setText("Total Song "+QString::number(total_songs));
        }
    }
}

void Music_Player::add_queue()
{
    QList < QTableWidgetItem * > SelectedItems;
    SelectedItems = song_table->selectedItems();
    foreach(QTableWidgetItem *itm, SelectedItems)
    {
        song_queue.append(itm->row());
        if(first_queue)
        {
            static auto queue_itr = song_queue.begin();
            first_queue = false;
        }
    }
}

void Music_Player::show_context_menu(QPoint pos)
{
    context_menu->popup(song_table->viewport()->mapToGlobal(pos));
}

void Music_Player::NewPlaylist()
{
    bool ok;
    QString PlayListName = QInputDialog::getText(this, "Create Playlist", "Playlist Name",
                                                 QLineEdit::Normal, "",&ok);

    if(ok && !PlayListName.isEmpty())
    {
        QList<QString>keys = playlist_action_container.keys();
        if(!playlist_action_container.isEmpty())
        {
            foreach(QString pl_name, keys)
            {
                if(pl_name == PlayListName)
                {
                    already = true;
                    QMessageBox::warning(this, "ERROR",
                                "A playlist already exists with same name.\n\n"
                                "Try another name");
                    break;
                }
            }
        }
        if(!already)
        {
            QMediaPlaylist *new_playlist = new QMediaPlaylist;
            QAction *playlist_action;
            QAction *addto_action;
            QString path = pl_path + "/";
            QList < QTableWidgetItem * > SelectedItems;
            SelectedItems = song_table->selectedItems();
            if(!SelectedItems.isEmpty())
            {
                foreach (QTableWidgetItem *itm, SelectedItems)
                {
                    QString file_path = path + itm->text();
                    new_playlist->addMedia(QUrl::fromLocalFile(file_path));
                    savePlaylistToFile(PlayListName, itm->text());
                    //status_bar->showMessage("Added", 3000);
                }

                playlist_action = new QAction(PlayListName);
                addto_action = new QAction(PlayListName);
                // store in in Key, Value pair
                playlist_action_container.insert(PlayListName, playlist_action);
                addto_action_container.insert(PlayListName, addto_action);
                playlist_container.insert(PlayListName, new_playlist);

                playlist_sub_menu->addAction(playlist_action_container[PlayListName]);
                add_to_sub_menu->addAction(addto_action_container[PlayListName]);

                playlist_action = nullptr;
                new_playlist = nullptr;
            }
        }
    }
    song_table->clearSelection();
    song_table->clearFocus();
    already = false;
}

void Music_Player::LoadPlaylist(QString PlaylistName)
{
    int total;

    QMediaPlaylist *playlist;
    playlist = playlist_container[PlaylistName];
    total = playlist->mediaCount();
    song_table->clear();
    song_table->setRowCount(0);
    song_table->setColumnCount(0);
    song_table->setRowCount(total);
    song_table->setColumnCount(1);
    for(int i=0; i<total; i++)
    {
        QMediaContent con = playlist->media(i);
        song_table->setItem(i, 0, new QTableWidgetItem(con.canonicalUrl().fileName()));
    }
    any_changes = false;
    total_songs = song_table->rowCount();
    song_player->setPlaylist(playlist);
    current_playing_lbl->setText("Song No. 1,");
    total_song_lbl->setText("Total Song "+QString::number(total_songs));
}

void Music_Player::AddTo(QAction *action)
{
    QMediaPlaylist *playlist;
    playlist = playlist_container[action->text()];
    QList < QTableWidgetItem * > SelectedItems;
    SelectedItems = song_table->selectedItems();
    if(!SelectedItems.isEmpty())
    {
        foreach(QTableWidgetItem *itm, SelectedItems)
        {
            QString file_name;
            file_name = pl_path + "/" + itm->text();
            playlist->addMedia(QUrl::fromLocalFile(file_name));
            savePlaylistToFile(action->text(), itm->text());
            status_bar->showMessage("Added", 3000);
        }
    }
}

void Music_Player::OpenPlaylist(QAction *action)
{
    QString PlaylistName = action->text();
    LoadPlaylist(PlaylistName);
}

void Music_Player::set_focus()
{
    song_table->clearSelection();
    song_table->clearFocus();
}

void Music_Player::set_music_layout()
{
    main_Hlayout = new QHBoxLayout;
    mControl_Vlayout = new QVBoxLayout;
    mControl_Hlayout = new QHBoxLayout;
    mDetail_Vlayout = new QVBoxLayout;
    mDetail_Hlayout = new QHBoxLayout;
    volControl_Hlayout = new QHBoxLayout;

    mControl_Hlayout->addWidget(shuffle_btn);
    mControl_Hlayout->addWidget(repeat_btn);
    mControl_Hlayout->addWidget(prev_btn);
    mControl_Hlayout->addWidget(play_pause_btn);
    mControl_Hlayout->addWidget(next_btn);

    mControl_Vlayout->addLayout(mControl_Hlayout);
    volControl_Hlayout->addWidget(vol_icon_btn);
    volControl_Hlayout->addWidget(volume_progress);
    mControl_Vlayout->addLayout(volControl_Hlayout);

    mDetail_Hlayout->addWidget(CURRENT_DUR);
    mDetail_Hlayout->addWidget(song_progress);
    mDetail_Hlayout->addWidget(TOTAL_DUR);
    mDetail_Hlayout->addWidget(info_btn);

    mDetail_Vlayout->addLayout(mDetail_Hlayout);
    mDetail_Vlayout->addWidget(currrent_play_lbl);

    main_Hlayout->addLayout(mControl_Vlayout);
    main_Hlayout->addLayout(mDetail_Vlayout);

    toolbar_container->setLayout(main_Hlayout);
    tool_bar->addWidget(toolbar_container);
}

void Music_Player::set_volume(int vol)
{
    if(!is_mute)
        song_player->setVolume(vol);
    else
        cur_volume = vol;
}

void Music_Player::set_style()
{
    main_container->setObjectName("main_con");
    song_table->setObjectName("song_tbl");
    toolbar_container->setObjectName("tool_con");
    song_progress->setObjectName("song_slider");
    CURRENT_DUR->setObjectName("curr_dur");
    TOTAL_DUR->setObjectName("tot_dur");
    status_bar->setObjectName("status_bar");
    song_table->verticalScrollBar()->setObjectName("v_scrollbar");
    currrent_play_lbl->setObjectName("play_lbl");
    song_table->setObjectName("song_tbl");
    current_playing_lbl->setObjectName("playing_lbl");
    total_song_lbl->setObjectName("tot_lbl");

    QFile read_style(":player/stylesheet/style.css");
    if(read_style.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&read_style);
        QString all_styles = in.readAll();
        setStyleSheet(all_styles);
        read_style.close();
    }
}

void Music_Player::song_clicked(QTableWidgetItem *song_item)
{
    if(song_item->row() == total_songs)
    {
        if(by_next)
            song_player->playlist()->setCurrentIndex(song_item->row());

        if(by_prev)
            song_player->playlist()->setCurrentIndex(total_songs);

        if(!by_next && !by_prev)
            song_player->playlist()->setCurrentIndex(song_item->row());

        by_next = false;
        by_prev = false;
    }

    else
    {
        song_player->playlist()->setCurrentIndex(song_item->row());
    }

    if(any_changes)
    {
        // restore previous color property
        prev_item->setForeground(fg_brush);
        prev_item->setBackground(bg_brush);
        prev_item->setTextColor(txt_clr);
    }

    prev_item = song_item;
    song_item->setBackgroundColor(bg_color);
    any_changes = true;
}

void Music_Player::set_song_name(QMediaContent con)
{
    currrent_play_lbl->setText(con.canonicalUrl().fileName());
    QString pl_no = "Song No. " + QString::number(main_playlist->currentIndex() + 1) + " ,";
    current_playing_lbl->setText(pl_no);
    media_resource = con.canonicalResource();
    generate_media_info();
}

void Music_Player::create_action_and_control()
{
    refresh_action = new QAction("Refresh");
    refresh_action->setIcon(QIcon(":player/icons/refresh.png"));
    refresh_action->setShortcut(tr("CTRL+R"));
    connect(refresh_action, SIGNAL(triggered()), this, SLOT(refresh_library()));
    player_menu->addAction(refresh_action);

    exit_action = new QAction("Quit");
    exit_action->setIcon(QIcon(":player/icons/exit.png"));
    exit_action->setShortcut(QKeySequence::Close);
    connect(exit_action, SIGNAL(triggered()), this, SLOT(exit_slot()));
    player_menu->addAction(exit_action);

    main_playlist_action = new QAction("Main Playlist");
    playlist_sub_menu->addAction(main_playlist_action);

    about_bat_player = new QAction("About");
    connect(about_bat_player, SIGNAL(triggered()), this, SLOT(about_player()));
    about_menu->addAction(about_bat_player);

    about_qt = new QAction("About Qt");
    connect(about_qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    about_menu->addAction(about_qt);

    play_pause_btn = new QPushButton;
    play_pause_btn->setToolTip("Play/Pause");
    play_pause_btn->setIcon(QIcon(":player/icons/pause.png"));
    play_pause_btn->setCursor(QCursor(Qt::PointingHandCursor));
    connect(play_pause_btn, SIGNAL(clicked()), this, SLOT(play_or_pause()));

    next_btn = new QPushButton;
    next_btn->setToolTip("Next Track");
    next_btn->setIcon(QIcon(":player/icons/next.png"));
    next_btn->setCursor(QCursor(Qt::PointingHandCursor));
    connect(next_btn, SIGNAL(clicked()), this, SLOT(next_track()));

    prev_btn = new QPushButton;
    prev_btn->setToolTip("Previous Track");
    prev_btn->setIcon(QIcon(":player/icons/previous.png"));
    prev_btn->setCursor(QCursor(Qt::PointingHandCursor));
    connect(prev_btn, SIGNAL(clicked()), this, SLOT(prev_track()));

    shuffle_btn = new QPushButton;
    shuffle_btn->setToolTip("Shuffle Off");
    shuffle_btn->setIcon(QIcon(":player/icons/shuffle_off.png"));
    shuffle_btn->setCursor(QCursor(Qt::PointingHandCursor));
    connect(shuffle_btn, SIGNAL(clicked()), this, SLOT(set_shuffle()));

    repeat_btn = new QPushButton;
    repeat_btn->setToolTip("No Repeat");
    repeat_btn->setIcon(QIcon(":player/icons/repeat.png"));
    repeat_btn->setCursor(QCursor(Qt::PointingHandCursor));
    connect(repeat_btn, SIGNAL(clicked()), this, SLOT(set_repeat()));

    vol_icon_btn = new QPushButton;
    vol_icon_btn->setFixedWidth(40);
    vol_icon_btn->setIcon(QIcon(":player/icons/volume.png"));
    vol_icon_btn->setToolTip("Mute/Unmute");
    vol_icon_btn->setCursor(QCursor(Qt::PointingHandCursor));
    connect(vol_icon_btn, SIGNAL(clicked()), this, SLOT(mute_unmute()));

    info_btn = new QPushButton;
    info_btn->setFixedWidth(40);
    info_btn->setIcon(QIcon(":player/icons/song-info.png"));
    info_btn->setToolTip("Song info");
    info_btn->setCursor(QCursor(Qt::PointingHandCursor));
    connect(info_btn, SIGNAL(clicked()), this, SLOT(show_song_info()));
}

void Music_Player::show_song_info()
{

    QMessageBox::information(this, "information", info);
}

void Music_Player::generate_media_info()
{
    QString codec = "Audio Codec: ";
    QString bitrate = "Audio bit rate: ";
    QString song_name = "Song: ";
    QString path = "Path: ";
    int bit_rate;
    QString a_codec;

    bit_rate = media_resource.audioBitRate();

    if(bit_rate == 0)
        bitrate += "NA";
    else
        bitrate += QString::number(bit_rate);

    a_codec = media_resource.audioCodec();

    if(a_codec.isNull())
        codec += "NA";
    else
        codec += a_codec;

    song_name += media_resource.url().fileName();
    path += media_resource.url().path();

    info = bitrate+"\n"+codec+"\n"+song_name+"\n"+path;
}

void Music_Player::mute_unmute()
{
    if(is_mute)
    {
        song_player->setVolume(cur_volume);
        vol_icon_btn->setIcon(QIcon(":player/icons/volume.png"));
        is_mute = false;
    }
    else
    {
        cur_volume = volume_progress->value();
        song_player->setVolume(0);
        vol_icon_btn->setIcon(QIcon(":player/icons/mute.png"));
        is_mute = true;
    }
}

void Music_Player::about_player()
{
    QMessageBox::about(this, "About", "<b>BAT-MUSIC-PLAYER V-1.0</b><br><br><br>"
                                      "This program is developed using c/c++(Qt Framework)"
                                      "anyone can use this program according to him/her."
                                      "There is no copyright or anyother shit!!(i mean issue)"
                                      "<br><br><br>"
                                      "<b>Happy Music</b>(A Batman Fan)");
}

void Music_Player::refresh_library()
{
    status_bar->showMessage("Refreshing", 4000);
    QFile read_set(".main_playlist.set");
    if(read_set.exists())
    {
        read_set.close();
        QDir folder(pl_path);
        all_songs.clear();
        all_songs = folder.entryList(QStringList()<<"*.mp3", QDir::Files);
        refresh = true;
        show_all_songs();
    }
    else
    {
        song_player->playlist()->clear();
        song_table->clear();
        song_table->setRowCount(0);
        song_table->setColumnCount(0);
        CURRENT_DUR->setText("0:0");
        TOTAL_DUR->setText("0:0");
        load_music();
    }

    status_bar->showMessage("Refreshed", 4000);
}

void Music_Player::load_music()
{
    QFile read_settings(".main_playlist.set");
    if(read_settings.exists())
    {
        display_songs();
    }
    else
    {
        if(add_folder_btn == nullptr && Vlayout1 == nullptr)
        {
            add_folder_btn = new QPushButton;
            add_folder_btn->setToolTip("Add Folder");
            add_folder_btn->setIconSize(QSize(100,100));
            add_folder_btn->setCursor(QCursor(Qt::PointingHandCursor));
            add_folder_btn->setIcon(QIcon(":player/icons/add_folder.png"));
            connect(add_folder_btn, SIGNAL(clicked()), this, SLOT(add_folder()));

            Vlayout1 = new QVBoxLayout;
            Vlayout1->addWidget(add_folder_btn);
            Vlayout1->setAlignment(Qt::AlignCenter);
            add_folder_btn->setStyleSheet("QPushButton{border:none}");
            main_container->setLayout(Vlayout1);
            currrent_play_lbl->setText("BAT-MUSIC-PLAYER");

            enable_control(NO);
        }
        if( centralWidget() )
        {
            centralWidget()->setParent(0);
            setCentralWidget(main_container);
        }
        else
            setCentralWidget(main_container);
    }
}

void Music_Player::set_shuffle()
{
    if(is_shuffle)
    {
        song_player->playlist()->setPlaybackMode(QMediaPlaylist::Sequential);
        shuffle_btn->setIcon(QIcon(":player/icons/shuffle_off.png"));
        shuffle_btn->setToolTip("Shuffle Off");
        is_shuffle = false;
    }
    else
    {
        song_player->playlist()->setPlaybackMode(QMediaPlaylist::Random);
        shuffle_btn->setIcon(QIcon(":player/icons/shuffle_on.png"));
        shuffle_btn->setToolTip("Shuffle On");
        is_shuffle = true;
    }
}

void Music_Player::set_repeat()
{
    // check that there is no repetition
    // if so... then repeat current track
    if(!repeat_all && !repeat_current && !no_repeat)
    {
        // set repetition of current playing track
        repeat(REPEAT::CURRENT);
        repeat_btn->setIcon(QIcon(":player/icons/repeat_cur.png"));
        repeat_btn->setToolTip("Repeat Current");
        // means current track is repeating
        repeat_current = true;
    }
    else
    {
        // current track is repeating
        if(repeat_current)
        {
            // set repetition of all track
            repeat(REPEAT::ALL);
            repeat_btn->setIcon(QIcon(":player/icons/repeat_all.png"));
            repeat_btn->setToolTip("Repeat All");
            // current track is not repeating
            repeat_current = false;
            // means all track is repeating
            repeat_all = true;
        }
        // all track is repeating
        else if(repeat_all)
        {
            // set no repetition
            repeat(REPEAT::NONE);
            repeat_btn->setIcon(QIcon(":player/icons/repeat.png"));
            repeat_btn->setToolTip("No Repeat");
            // no repetition
            repeat_all = false;
        }
    }
}

void Music_Player::repeat(const REPEAT& RV)
{
    switch (RV)
    {
        case REPEAT::ALL:
            song_player->playlist()->setPlaybackMode(QMediaPlaylist::Loop);
            break;

        case REPEAT::CURRENT:
            song_player->playlist()->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);
            break;

        case REPEAT::NONE:
            song_player->playlist()->setPlaybackMode(QMediaPlaylist::CurrentItemOnce);
            break;
    }
}

void Music_Player::add_folder()
{
    QString f_path = QFileDialog::getExistingDirectory(this, "select folder");
    if(!f_path.isEmpty())
    {
        QFile save_set(".main_playlist.set");
        if(save_set.open(QIODevice::WriteOnly))
        {
            QDataStream out(&save_set);
            all_settings.playlist_path = f_path;
            out << all_settings.duration << all_settings.playlist_path
                << all_settings.row_no << all_settings.volume;
            save_set.flush();
            save_set.close();
        }
        QDir folder(f_path);
        QString path = f_path + "/";
        pl_path = f_path;
        main_playlist->clear();
        all_songs.clear();
        QStringList fltr;
        fltr<<"*.mp3"<<"*.MP3"<<"*.3gpp"<<"*.pcm"<<"*.PCM"
           <<"*.3GPP"<<"*.m4a"<<"*.M4A"<<"*.wav"<<"*.WAV"
           <<"*.aiff"<<"*.AIFF"<<"*.aac"<<"*.AAC"<<"*.ogg"
           <<"*.OGG"<<"*.wma"<<"*.WMA";
        all_songs = folder.entryList(fltr, QDir::Files);
        for(int i=0; i<all_songs.length(); ++i)
        {
            QString song_name = path + all_songs[i];
            main_playlist->addMedia(QUrl::fromLocalFile(song_name));
        }

        song_player->setPlaylist(main_playlist);
        delete add_folder_btn;
        delete Vlayout1;
        Vlayout1 = nullptr;
        add_folder_btn = nullptr;
        no_folder_selected = false;
        show_all_songs();
    }
    else
    {
       no_folder_selected = true;
       load_music();
    }
}

void Music_Player::show_all_songs()
{
    song_table->clear();
    if(refresh)
    {
        song_table->setRowCount(0);
        song_table->setColumnCount(0);
    }
    song_table->setRowCount(all_songs.length());
    song_table->setColumnCount(1);
    for(int i=0; i<all_songs.length(); i++)
    {
        song_table->setItem(i, 0, new QTableWidgetItem(all_songs[i]));
    }
    // color backup
    fg_brush = song_table->item(0,0)->foreground();
    bg_brush = song_table->item(0,0)->background();
    if(refresh)
    {
        if(song_player->state() == QMediaPlayer::PlayingState)
            song_player->pause();
        //main_playlist->clear();
        QString s_path = pl_path + "/";
        for(int i=0; i<all_songs.length(); i++)
            main_playlist->addMedia(QUrl::fromLocalFile(s_path+all_songs[i]));

        refresh = false;
        from_refresh = true;
    }

    if( centralWidget() )
    {
        if(!from_refresh)
        {
            centralWidget()->setParent(0);
            setCentralWidget(song_table);
        }
        from_refresh = false;
    }
    else
        setCentralWidget(song_table);

    enable_control(YES);

    status_bar->showMessage("Playlist loaded...", 4000);
    QString tot_no = "Total Song " + QString::number(all_songs.length());
    total_song_lbl->setText(tot_no);
    no_folder_selected = false;
    total_songs = song_table->rowCount();

    readPlaylistFromFile();
}

void Music_Player::enable_control(bool flag)
{
    if(flag)
    {
        // enable all control
        shuffle_btn->setEnabled(true);
        repeat_btn->setEnabled(true);
        prev_btn->setEnabled(true);
        play_pause_btn->setEnabled(true);
        next_btn->setEnabled(true);
        vol_icon_btn->setEnabled(true);
        volume_progress->setEnabled(true);
        song_progress->setEnabled(true);
        refresh_action->setEnabled(true);
    }
    else
    {
        // disable all control
        shuffle_btn->setDisabled(true);
        repeat_btn->setDisabled(true);
        prev_btn->setDisabled(true);
        play_pause_btn->setDisabled(true);
        next_btn->setDisabled(true);
        vol_icon_btn->setDisabled(true);
        volume_progress->setDisabled(true);
        song_progress->setDisabled(true);
        refresh_action->setDisabled(true);
    }
}

void Music_Player::display_songs()
{
    QFile read_path(".main_playlist.set");
    if(read_path.open(QIODevice::ReadOnly))
    {
        QDataStream in(&read_path);
        in >> all_settings.duration >> all_settings.playlist_path
           >> all_settings.row_no >> all_settings.volume;
        read_path.close();
        all_songs.clear();
        main_playlist->clear();
        pl_path = all_settings.playlist_path;
        QDir dir(pl_path);

        QStringList fltr;
        fltr<<"*.mp3"<<"*.MP3"<<"*.3gpp"<<"*.pcm"<<"*.PCM"
           <<"*.3GPP"<<"*.m4a"<<"*.M4A"<<"*.wav"<<"*.WAV"
           <<"*.aiff"<<"*.AIFF"<<"*.aac"<<"*.AAC"<<"*.ogg"
           <<"*.OGG"<<"*.wma"<<"*.WMA";
        all_songs = dir.entryList(fltr, QDir::Files);
        for(int i=0; i<all_songs.length(); ++i)
        {
            QString ss = pl_path + "/" + all_songs[i];
            main_playlist->addMedia(QUrl::fromLocalFile(ss));
        }
    }

    song_player->setPlaylist(main_playlist);
    show_all_songs();

    if(all_settings.duration <= NO_CHANGE)
        all_settings.duration = 0;
    if(all_settings.row_no <= NO_CHANGE)
        all_settings.row_no = 0;
    if(all_settings.volume <= NO_CHANGE)
        all_settings.volume = 100;

    // restore previous settings
    QTableWidgetItem *itm = song_table->item(all_settings.row_no, 0);
    volume_progress->setValue(all_settings.volume);
    //song_clicked(itm);
    main_playlist->setCurrentIndex(itm->row());
    itm->setBackgroundColor(bg_color);
    prev_item = itm;
    forwarded(all_settings.duration);
    no_folder_selected = false;
    any_changes = true;
}

void Music_Player::total_dur(qint64 dur)
{
    int total_sec = dur / 1000;
    int minute = total_sec / 60;
    int sec = total_sec % 60;
    QString total_time = QString::number(minute) + ":" + QString::number(sec);
    TOTAL_DUR->setText(total_time);

    song_progress->setMaximum(total_sec);
    song_progress->setValue(0);
}

void Music_Player::current_dur(qint64 dur)
{
    int cur_sec = dur / 1000;
    int minute = cur_sec / 60;
    int sec = cur_sec % 60;
    QString cur_time = QString::number(minute) + ":" + QString::number(sec);
    CURRENT_DUR->setText(cur_time);

    song_progress->setValue(cur_sec);
}

void Music_Player::play_or_pause()
{
    if(song_player->state() == QMediaPlayer::PlayingState)
    {
        song_player->pause();
        play_pause_btn->setIcon(QIcon(":player/icons/pause.png"));
    }
    else
    {
        song_player->play();
        play_pause_btn->setIcon(QIcon(":player/icons/play.png"));
    }
}

void Music_Player::next_track()
{
    QTableWidgetItem *p_item;
    short cur_row = song_player->playlist()->currentIndex();
    song_player->playlist()->next();
    if(cur_row == total_songs)
    {
        p_item = song_table->item(0, 0);
    }
    else
        p_item = song_table->item(song_player->playlist()->currentIndex(), 0);
    by_next = true;
    auto_changed = false;
    if(!song_queue.isEmpty())
        song_queue.clear();
    song_clicked(p_item);
}

void Music_Player::prev_track()
{
    QTableWidgetItem *p_item;
    short cur_row = song_player->playlist()->currentIndex();
    song_player->playlist()->previous();
    if(cur_row == 0)
    {
        p_item = song_table->item(song_table->rowCount()-1, 0);
    }
    else
        p_item = song_table->item(song_player->playlist()->currentIndex(), 0);

    by_prev = true;
    auto_changed = false;
    if(!song_queue.isEmpty())
        song_queue.clear();
    song_clicked(p_item);
}

void Music_Player::forwarded(int dur)
{
    song_player->setPosition(dur * 1000);
}

void Music_Player::exit_slot()
{
    force_quit = true;
    close();
}

void Music_Player::savePlaylistToFile(const QString& pl_name, const QString& s_name)
{
    QString playlist_name = pl_name + ".pl";
    QFile file(playlist_name);
    if(!file.exists())
    {
        file.open(QIODevice::WriteOnly);
        QTextStream out(&file);
        out<<pl_name + "\n";
        file.close();
    }
    if(file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QTextStream out(&file);
        out<<pl_path + "/" + s_name + "\n";
        file.close();
    }
}

void Music_Player::readPlaylistFromFile()
{
    QDir dir(".");
    QStringList all_pl;
    all_pl = dir.entryList(QStringList()<<"*.pl", QDir::Files);
    // create actions
    for(int i=0; i<all_pl.length(); ++i)
    {
        QString pl_name;
        QFile file(all_pl[i]);
        if(file.open(QIODevice::ReadOnly))
        {
            QTextStream in(&file);
            pl_name = in.readLine();

            playlist_action_container.insert(pl_name, new QAction(pl_name));
            addto_action_container.insert(pl_name, new QAction(pl_name));

            playlist_sub_menu->addAction(playlist_action_container[pl_name]);
            add_to_sub_menu->addAction(addto_action_container[pl_name]);

            QMediaPlaylist *pl_list = new QMediaPlaylist;
            playlist_container.insert(pl_name, pl_list);

            while(!in.atEnd())
            {
                QString song;
                song = in.readLine();
                pl_list->addMedia(QUrl::fromLocalFile(song));
            }

            file.close();
        }
    }
}

void Music_Player::closeEvent(QCloseEvent *event)
{
    if(song_player->state() == QMediaPlayer::PlayingState)
    {
        if(force_quit)
        {
            if(!no_folder_selected)
            {
                all_settings.playlist_path = pl_path;
                all_settings.duration = song_progress->value();
                all_settings.volume = volume_progress->value();
                all_settings.row_no = main_playlist->currentIndex();

                QFile save_settings(".bat_player/settings/.main_playlist.set");
                if(save_settings.open(QIODevice::WriteOnly))
                {
                    QDataStream out(&save_settings);
                    out << all_settings.duration << all_settings.playlist_path
                        << all_settings.row_no << all_settings.volume;
                    save_settings.flush();
                    save_settings.close();
                }
            }
            event->accept();
        }
        else
        {
            event->ignore();
            showMinimized();
        }
    }
    else
    {
        if(!no_folder_selected)
        {
            all_settings.playlist_path = pl_path;
            all_settings.duration = song_progress->value();
            all_settings.volume = volume_progress->value();
            all_settings.row_no = main_playlist->currentIndex();

            QFile save_settings(".bat_player/settings/.main_playlist.set");
            if(save_settings.open(QIODevice::WriteOnly))
            {
                QDataStream out(&save_settings);
                out << all_settings.duration << all_settings.playlist_path
                    << all_settings.row_no << all_settings.volume;
                save_settings.flush();
                save_settings.close();
            }
        }
        event->accept();
    }
}

Music_Player::~Music_Player()
{
    delete player_menu;
    delete playlist_menu;
    delete about_menu;
    delete context_menu;
    delete exit_action;
    delete refresh_action;
    delete tool_bar;
    delete status_bar;
    delete song_player;

    if(Vlayout1 != nullptr && add_folder_btn != nullptr)
        delete Vlayout1;

    delete main_container;
    delete main_playlist_action;
    delete about_bat_player;
    delete about_qt;
}

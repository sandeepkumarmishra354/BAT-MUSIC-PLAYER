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
#include <QPropertyAnimation>

Music_Player::Music_Player(QWidget *parent):QMainWindow(parent)
{
    bg_color.setNamedColor("#8080ff");
    txt_clr.setNamedColor("white");

    player_menu = menuBar()->addMenu(tr("&BAT-PLAYER"));
    playlist_menu = menuBar()->addMenu(tr("&Playlist"));
    playlist_sub_menu = playlist_menu->addMenu("Open Playlist");
    connect(playlist_sub_menu, SIGNAL(triggered(QAction*)), this, SLOT(OpenPlaylist(QAction*)));
    remove_playlist_sub_menu = playlist_menu->addMenu("Remove Playlist");
    connect(remove_playlist_sub_menu, SIGNAL(triggered(QAction*)), this, SLOT(removePlaylist(QAction*)));
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
    connect(song_player, SIGNAL(metaDataAvailableChanged(bool)), this, SLOT(metaInfo(bool)));
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
    song_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(song_table, SIGNAL(cellClicked(int,int)), this, SLOT(set_focus()));
    connect(song_table, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(song_clicked(QTableWidgetItem*)));
    connect(song_table, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(show_context_menu(QPoint)));

    total_song_lbl = new QLabel;
    current_playing_lbl = new QLabel;
    status_bar->addPermanentWidget(current_playing_lbl);
    status_bar->addPermanentWidget(total_song_lbl);

    mediaInfo = new Media_info(this);
    timer = new Timer(this);

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

    playlist_container.insert("All Songs", main_playlist);

    setWindowIcon(QIcon(":player/icons/Bat_player.ico"));
    setMinimumWidth(800);
    setMinimumHeight(650);
    setWindowTitle("BAT-MUSIC-PLAYER");
    showMaximized();
    setFocus();

    fade_effect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(fade_effect);
    QPropertyAnimation *animation = new QPropertyAnimation(fade_effect, "opacity");
    animation->setEasingCurve(QEasingCurve::InOutQuad);
    animation->setDuration(3000);
    animation->setStartValue(0.001);
    animation->setEndValue(1.0);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
    setVisible(true);
}

void Music_Player::create_context_menu()
{
    context_menu = new QMenu(this);
    context_menu->setStyleSheet("background-color:#8080ff");
    create_new_playlist = new QAction("Create New Playlist");
    connect(create_new_playlist, SIGNAL(triggered()), this, SLOT(NewPlaylist()));
    context_menu->addAction(create_new_playlist);

    add_to_sub_menu = context_menu->addMenu("Add To");
    connect(add_to_sub_menu, SIGNAL(triggered(QAction*)), this, SLOT(AddTo(QAction*)));

    add_to_queue = new QAction("Add to Queue");
    context_menu->addAction(add_to_queue);
    connect(add_to_queue, SIGNAL(triggered()), this, SLOT(add_queue()));
    add_to_queue->setDisabled(true);

    remove_song = new QAction("Remove");
    remove_song->setToolTip("Remove song from current playlist");
    connect(remove_song, SIGNAL(triggered()), this, SLOT(remove_()));
    context_menu->addAction(remove_song);

    delete_song = new QAction("Delete");
    delete_song->setToolTip("Delete song from disk storage");
    connect(delete_song, SIGNAL(triggered()), this, SLOT(delete_()));
    context_menu->addAction(delete_song);
}

void Music_Player::metaInfo(bool status)
{
    if(status)
    {
        QString Artist = song_player->metaData(QMediaMetaData::AlbumArtist).toString();
        mediaInfo->artistName(Artist);
    }
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
        status_bar->showMessage("removed...",4000);
    }
}

void Music_Player::delete_()
{
    int option;
    option = QMessageBox::warning(this, "Delete Permanantly", "Are you sure ?",
                                  QMessageBox::Ok, QMessageBox::Cancel);
    if(option == QMessageBox::Ok)
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
            status_bar->showMessage("deleted...",4000);
        }
    }
}

void Music_Player::add_queue()
{
    /*
    QList <QTableWidgetItem*> SelectedItems;
    SelectedItems = song_table->selectedItems();
    foreach(QTableWidgetItem *itm, SelectedItems)
    {
        song_queue.append(itm->row());
    }
    */
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
        QList<QString>keys = playlist_container.keys();
        if(!playlist_action_container.isEmpty())
        {
            foreach(QString pl_name, keys)
            {
                if(PlayListName == pl_name)
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
            QAction *remove_action;
            //QString path = pl_path + "/";
            QList < QTableWidgetItem * > SelectedItems;
            SelectedItems = song_table->selectedItems();
            if(!SelectedItems.isEmpty())
            {
                songToSave.clear();
                QMediaPlaylist *pl = song_player->playlist();
                foreach (QTableWidgetItem *itm, SelectedItems)
                {
                    QString songWithPath = pl->media(itm->row()).canonicalUrl().path();
                    //QString songTitle = pl->media(itm->row()).canonicalUrl().fileName();
                    new_playlist->addMedia(QUrl::fromLocalFile(songWithPath));
                    songToSave.append(songWithPath);
                    //savePlaylistToFile(PlayListName, songWithPath);
                }
                savePlaylistToFile(PlayListName, songToSave);

                playlist_action = new QAction(PlayListName);
                addto_action = new QAction(PlayListName);
                remove_action = new QAction(PlayListName);
                // store in in Key, Value pair
                playlist_action_container.insert(PlayListName, playlist_action);
                remove_action_container.insert(PlayListName, remove_action);
                addto_action_container.insert(PlayListName, addto_action);
                playlist_container.insert(PlayListName, new_playlist);

                playlist_sub_menu->addAction(playlist_action_container[PlayListName]);
                remove_playlist_sub_menu->addAction(remove_action_container[PlayListName]);
                add_to_sub_menu->addAction(addto_action_container[PlayListName]);

                playlist_action = nullptr;
                new_playlist = nullptr;
                remove_action = nullptr;
                status_bar->showMessage("done...",4000);
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
    status_bar->showMessage("loaded...",4000);
}

void Music_Player::AddTo(QAction *action)
{
    QMediaPlaylist *playlist;
    playlist = playlist_container[action->text()];
    QList < QTableWidgetItem * > SelectedItems;
    SelectedItems = song_table->selectedItems();
    if(!SelectedItems.isEmpty())
    {
        songToSave.clear();
        QMediaPlaylist *t_p = song_player->playlist();
        QMediaContent con;
        foreach(QTableWidgetItem *itm, SelectedItems)
        {
            con = t_p->media(itm->row());
            QString file_name = con.canonicalUrl().path();
            playlist->addMedia(QUrl::fromLocalFile(file_name));
            songToSave.append(file_name);
        }
        savePlaylistToFile(action->text(), songToSave);
        status_bar->showMessage("Added...", 3000);
    }
}

void Music_Player::OpenPlaylist(QAction *action)
{
    QString PlaylistName = action->text();
    LoadPlaylist(PlaylistName);
}

void Music_Player::removePlaylist(QAction *action)
{
    QString PlaylistName = action->text();
    QMediaPlaylist *curPlylist = song_player->playlist();
    QMediaPlaylist *plylistToRemove = playlist_container[PlaylistName];
    if(curPlylist == plylistToRemove)
        LoadPlaylist("All Songs");// loads main playlist

    delete playlist_container[PlaylistName];
    delete addto_action_container[PlaylistName];
    delete remove_action_container[PlaylistName];
    delete playlist_action_container[PlaylistName];

    playlist_container.remove(PlaylistName);
    addto_action_container.remove(PlaylistName);
    remove_action_container.remove(PlaylistName);
    playlist_action_container.remove(PlaylistName);

    QFile file(PlaylistName+".pl");
    if(file.exists())
    {
        if(file.remove())
            status_bar->showMessage("Playlist Removed...",4000);
    }
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
    mDetail_Hlayout->addWidget(search_bar);

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
    song_queue.clear();
    queueIndex = 0;
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
    mediaInfo->currentMedia(con.canonicalResource());
    //justNow = false;
}
/*
void Music_Player::setSongLabelByQueue(const QMediaContent &con) const
{
    currrent_play_lbl->setText(con.canonicalUrl().fileName());
    QString pl_no = "Song No. " + QString::number(main_playlist->currentIndex() + 1) + " ,";
    current_playing_lbl->setText(pl_no);
    mediaInfo->currentMedia(con.canonicalResource());
}
*/
void Music_Player::create_action_and_control()
{
    allSongs = new QAction("All Songs");
    allSongs->setShortcut(tr("CTRL+L"));
    allSongs->setIcon(QIcon(":player/icons/all-songs.png"));
    connect(allSongs, SIGNAL(triggered()), this, SLOT(loadAllSongs()));
    player_menu->addAction(allSongs);

    addMoreFolder = new QAction("Add folder");
    addMoreFolder->setIcon(QIcon(":player/icons/add_folder.png"));
    addMoreFolder->setShortcut(tr("CTRL+F"));
    connect(addMoreFolder, SIGNAL(triggered()), this, SLOT(addFolder()));
    player_menu->addAction(addMoreFolder);

    refresh_action = new QAction("Refresh");
    refresh_action->setIcon(QIcon(":player/icons/refresh.png"));
    refresh_action->setShortcut(tr("CTRL+R"));
    connect(refresh_action, SIGNAL(triggered()), this, SLOT(refresh_library()));
    player_menu->addAction(refresh_action);

    timerAction = new QAction("Set Timer");
    timerAction->setShortcut(tr("CTRL+T"));
    timerAction->setIcon(QIcon(":player/icons/timer.png"));
    connect(timerAction, SIGNAL(triggered()), this, SLOT(setTimeOut()));
    player_menu->addAction(timerAction);

    exit_action = new QAction("Quit");
    exit_action->setIcon(QIcon(":player/icons/exit.png"));
    exit_action->setShortcut(QKeySequence::Close);
    connect(exit_action, SIGNAL(triggered()), this, SLOT(exit_slot()));
    player_menu->addAction(exit_action);

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

    search_bar = new QLineEdit;
    search_bar->setMaximumWidth(150);
    search_bar->setToolTip("Search songs");
    search_bar->setPlaceholderText("search");
    connect(search_bar, SIGNAL(textChanged(QString)), this, SLOT(search(QString)));
}

void Music_Player::setTimeOut()
{
    qDebug()<<"TIMER";
    timer->exec();
}

void Music_Player::timeOut()
{
    exit_slot();
}

void Music_Player::addFolder()
{
    QString selectedDir;
    QDir dir;
    QStringList songs, filter;
    selectedDir = QFileDialog::getExistingDirectory(this, "select folder", "/root");
    if(!selectedDir.isEmpty())
    {
        filter<<"*.mp3"<<"*.MP3"<<"*.3gpp"<<"*.pcm"<<"*.PCM"
              <<"*.3GPP"<<"*.m4a"<<"*.M4A"<<"*.wav"<<"*.WAV"
              <<"*.aiff"<<"*.AIFF"<<"*.aac"<<"*.AAC"<<"*.ogg"
              <<"*.OGG"<<"*.wma"<<"*.WMA";

        dir.setPath(selectedDir);
        songs = dir.entryList(filter, QDir::Files);
        if(!songs.isEmpty())
        {
            QMediaPlaylist *mainPl;
            mainPl = playlist_container["All Songs"];
            int tot = mainPl->mediaCount();
            bool flag = true;
            foreach(QString itm, songs)
            {
                QString song_title = itm;
                itm.prepend(selectedDir+"/");
                for(int i=0; i<tot; ++i)
                {
                    QString f_s = mainPl->media(i).canonicalUrl().path();
                    //qDebug()<<f_s;
                    if(f_s == itm)
                    {
                        flag = false;
                        break;
                    }
                }
                if(flag)
                {
                    mainPl->addMedia(QUrl::fromLocalFile(itm));
                    song_table->insertRow(song_table->rowCount());
                    song_table->setItem(song_table->rowCount()-1,0,new QTableWidgetItem(song_title));
                    status_bar->showMessage("added...",4000);
                }
                flag = true;
            }
            song_table->sortItems(Qt::AscendingOrder);
            QString tot_no = "Total Song " + QString::number(song_table->rowCount());
            total_song_lbl->setText(tot_no);
        }

        if(!songs.isEmpty())
        {
            QFile file(".main_playlist.set");
            QString JSONstring;
            if(file.open(QIODevice::ReadOnly))
            {
                JSONstring = file.readAll();
                file.close();
            }
            if(file.open(QIODevice::WriteOnly))
            {
                bool folderAlready = false;
                QJsonDocument jDoc = QJsonDocument::fromJson(JSONstring.toUtf8());
                QJsonObject obj = jDoc.object();
                QJsonArray jsArr = obj["folderPaths"].toArray();
                foreach(auto itm, jsArr)
                {
                    if(itm.toString() == selectedDir)
                    {
                        qDebug()<<"ALREADY";
                        folderAlready = true;
                        break;
                    }
                }
                if(!folderAlready)
                {
                    jsArr.append(QJsonValue(selectedDir));
                    obj["folderPaths"] = jsArr;
                    JSONstring = QJsonDocument(obj).toJson();
                    QTextStream out(&file);
                    out<<JSONstring;
                }
                else
                {
                    obj["folderPaths"] = jsArr;
                    JSONstring = QJsonDocument(obj).toJson();
                    QTextStream out(&file);
                    out<<JSONstring;
                }
                file.close();
            }
        }
    }
}

void Music_Player::loadAllSongs()
{
    OpenPlaylist(allSongs);
}

void Music_Player::show_song_info()
{
    if(mediaInfo != nullptr)
    {
        mediaInfo->exec();
    }
}

void Music_Player::search(const QString& textToSearch)
{
    for(int i=0; i<song_table->rowCount(); ++i)
        song_table->hideRow(i);
    QList<QTableWidgetItem*> itms;
    itms = song_table->findItems(textToSearch, Qt::MatchContains);
    foreach(auto itm, itms)
    {
        song_table->showRow(itm->row());
    }
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
                                      "There is no copyright or anyother shit!!(i mean issue)."
                                      "<br>For source code "
                                      "<a href='https://github.com/sandeepkumarmishra354/BAT-MUSIC-PLAYER'>"
                                      "click here</a><br>"
                                      "Email: <a href='mailto:sandeepkumarmishra354@gmail.com'>"
                                      "sandeepkumarmishra354@gmail.com</a>"
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

    status_bar->showMessage("Refreshed...", 4000);
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
            QJsonObject rootObj;
            QJsonArray dirPath;
            dirPath.append(QJsonValue(f_path));
            rootObj["folderPaths"] = dirPath;
            rootObj["volume"] = 100;
            rootObj["duration"] = 0;
            rootObj["songNo"] = 0;
            QString all_set = QJsonDocument(rootObj).toJson(QJsonDocument::Indented);
            QTextStream out(&save_set);
            out<<all_set;
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
        totalRow = all_songs.length();
        foreach(QString itm, all_songs)
        {
            main_playlist->addMedia(QUrl::fromLocalFile(path + itm));
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
    song_table->setRowCount(totalRow);
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
        for(int i=0; i<totalRow; i++)
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

    status_bar->showMessage("All Songs Loaded...", 4000);
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
        info_btn->setEnabled(true);
        playlist_sub_menu->setEnabled(true);
        remove_playlist_sub_menu->setEnabled(true);
        search_bar->setEnabled(true);
        allSongs->setEnabled(true);
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
        info_btn->setDisabled(true);
        playlist_sub_menu->setDisabled(true);
        remove_playlist_sub_menu->setDisabled(true);
        search_bar->setDisabled(true);
        allSongs->setDisabled(true);
    }
}

void Music_Player::display_songs()
{
    QJsonObject rootObj;
    QFile read_path(".main_playlist.set");
    if(read_path.open(QIODevice::ReadOnly))
    {
        QTextStream in(&read_path);
        QJsonParseError jErr;
        QJsonDocument jDoc = QJsonDocument::fromJson(read_path.readAll(), &jErr);
        read_path.close();
        rootObj = jDoc.object();
        all_songs.clear();
        main_playlist->clear();
        QStringList fltr;
        fltr<<"*.mp3"<<"*.MP3"<<"*.3gpp"<<"*.pcm"<<"*.PCM"
            <<"*.3GPP"<<"*.m4a"<<"*.M4A"<<"*.wav"<<"*.WAV"
            <<"*.aiff"<<"*.AIFF"<<"*.aac"<<"*.AAC"<<"*.ogg"
            <<"*.OGG"<<"*.wma"<<"*.WMA";
        QJsonArray jsArr = rootObj["folderPaths"].toArray();
        QStringList tmpList;
        foreach(auto itm, jsArr)
        {
            pl_path = itm.toString();
            QDir dir(pl_path);
            tmpList = dir.entryList(fltr, QDir::Files);
            totalRow += tmpList.length();
            for(int i=0; i<tmpList.length(); ++i)
            {
                all_songs.append(tmpList[i]);
                QString ss = pl_path + "/" + tmpList[i];
                main_playlist->addMedia(QUrl::fromLocalFile(ss));
            }
        }
    }

    song_player->setPlaylist(main_playlist);
    show_all_songs();
    // restore previous settings
    int vol = rootObj["volume"].toInt();
    int dur = rootObj["duration"].toInt();
    int songRow = rootObj["songNo"].toInt();
    QTableWidgetItem *itm = song_table->item(songRow, 0);
    volume_progress->setValue(vol);
    //song_clicked(itm);
    main_playlist->setCurrentIndex(itm->row());
    itm->setBackgroundColor(bg_color);
    prev_item = itm;
    forwarded(dur);
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
    mediaInfo->totalDuration(TOTAL_DUR);
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

void Music_Player::savePlaylistToFile(const QString& playlistName, const QStringList& songList)
{
    QString pl_file = playlistName + ".pl";
    QFile file(pl_file);
    QJsonDocument jDoc;
    QJsonArray jArr;
    QJsonObject obj;
    if(!file.exists())
    {
        foreach(auto itm, songList)
            jArr.append(QJsonValue(itm));
        obj["playlist_name"] = playlistName;
        obj["songs"] = jArr;
        QString JSONfile = QJsonDocument(obj).toJson(QJsonDocument::Indented);

        if(file.open(QIODevice::WriteOnly))
        {
            QTextStream out(&file);
            out<<JSONfile;
            file.close();
        }
    }
    else
    {
        QString JSONfile;
        if(file.open(QIODevice::ReadOnly))
        {
            JSONfile = file.readAll();
            file.close();
        }
        jDoc = QJsonDocument::fromJson(JSONfile.toUtf8());
        obj = jDoc.object();
        jArr = obj["songs"].toArray();
        foreach (auto itm, songList)
            jArr.append(QJsonValue(itm));
        JSONfile = QJsonDocument(obj).toJson(QJsonDocument::Indented);
        if(file.open(QIODevice::WriteOnly))
        {
            QTextStream out(&file);
            out<<JSONfile;
            file.close();
        }
    }
    //songToSave.clear();
}

void Music_Player::readPlaylistFromFile()
{
    QDir dir(".");
    QStringList allPlaylist;
    QJsonDocument jDoc;
    QJsonArray jArr;
    QJsonObject obj;
    allPlaylist = dir.entryList(QStringList()<<"*.pl", QDir::Files);
    // create actions
    for(int i=0; i<allPlaylist.length(); ++i)
    {
        QString playlistName;
        QFile file(allPlaylist[i]);
        if(file.open(QIODevice::ReadOnly))
        {
            jDoc = QJsonDocument::fromJson(file.readAll());
            file.close();
            obj = jDoc.object();
            playlistName = obj["playlist_name"].toString();
            jArr = obj["songs"].toArray();

            playlist_action_container.insert(playlistName, new QAction(playlistName));
            remove_action_container.insert(playlistName, new QAction(playlistName));
            addto_action_container.insert(playlistName, new QAction(playlistName));

            playlist_sub_menu->addAction(playlist_action_container[playlistName]);
            remove_playlist_sub_menu->addAction(remove_action_container[playlistName]);
            add_to_sub_menu->addAction(addto_action_container[playlistName]);

            QMediaPlaylist *pl_list = new QMediaPlaylist;
            playlist_container.insert(playlistName, pl_list);

            foreach(auto itm, jArr)
            {
                QString song = itm.toString();
                file.setFileName(song);
                if(file.exists())
                    pl_list->addMedia(QUrl::fromLocalFile(song));
            }
        }
    }
}

void Music_Player::keyPressEvent(QKeyEvent *event)
{
    if(event->modifiers() == Qt::ControlModifier)
    {
        switch (event->key())
        {
            case Qt::Key_Left:
                forwarded(song_progress->value()-7);
                break;
            case Qt::Key_Right:
                forwarded(song_progress->value()+7);
                break;
            case Qt::Key_Up:
                set_volume(volume_progress->value()+5);
                volume_progress->setValue(volume_progress->value()+5);
                break;
            case Qt::Key_Down:
                set_volume(volume_progress->value()-5);
                volume_progress->setValue(volume_progress->value()-5);
                break;
        }
    }
/*
    if(event->key() == Qt::Key_Escape)
        exit_slot();
    if(event->key() == Qt::Key_Space)
        play_or_pause();
    if(event->key() == Qt::Key_Left)
        prev_track();
    if(event->key() == Qt::Key_Right)
        next_track();
*/
}

void Music_Player::keyReleaseEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape)
        exit_slot();
    if(event->key() == Qt::Key_Space)
        play_or_pause();
    if(event->key() == Qt::Key_Left)
        prev_track();
    if(event->key() == Qt::Key_Right)
        next_track();
}

void Music_Player::closeEvent(QCloseEvent *event)
{
    if(song_player->state() == QMediaPlayer::PlayingState)
    {
        if(force_quit)
        {
            if(!no_folder_selected)
            {
                int dur = song_progress->value();
                int vol = volume_progress->value();
                int songRow = main_playlist->currentIndex();
                QString f_value;
                QFile save_settings(".main_playlist.set");
                if(save_settings.open(QIODevice::ReadOnly))
                {
                    f_value = save_settings.readAll();
                    save_settings.close();
                }
                if(save_settings.open(QIODevice::WriteOnly))
                {
                    QJsonParseError jErr;
                    QJsonDocument jDoc = QJsonDocument::fromJson(f_value.toUtf8(), &jErr);
                    QJsonObject obj;
                    obj = jDoc.object();
                    obj["volume"] = vol;
                    obj["duration"] = dur;
                    obj["songNo"] = songRow;
                    QString JSON = QJsonDocument(obj).toJson();
                    QTextStream out(&save_settings);
                    out<<JSON;
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
            int dur = song_progress->value();
            int vol = volume_progress->value();
            int songRow = main_playlist->currentIndex();
            QString f_value;
            QFile save_settings(".main_playlist.set");
            if(save_settings.open(QIODevice::ReadOnly))
            {
                f_value = save_settings.readAll();
                save_settings.close();
            }
            if(save_settings.open(QIODevice::WriteOnly))
            {
                QJsonParseError jErr;
                QJsonDocument jDoc = QJsonDocument::fromJson(f_value.toUtf8(), &jErr);
                QJsonObject obj;
                obj = jDoc.object();
                obj["volume"] = vol;
                obj["duration"] = dur;
                obj["songNo"] = songRow;
                QString JSON = QJsonDocument(obj).toJson();
                QTextStream out(&save_settings);
                out<<JSON;
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
    delete allSongs;
    delete exit_action;
    delete refresh_action;
    delete tool_bar;
    delete status_bar;
    delete song_player;
    if(Vlayout1 != nullptr && add_folder_btn != nullptr)
        delete Vlayout1;
    delete main_container;
    delete about_bat_player;
    delete about_qt;
    if(mediaInfo != nullptr)
        delete mediaInfo;
    delete timer;
    delete fade_effect;
}

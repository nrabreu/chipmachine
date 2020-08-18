#include "ChipMachine.h"
#include "modutils.h"

#include <coreutils/environment.h>

namespace chipmachine {

void ChipMachine::setupCommands()
{
    using namespace tween;

    auto cmd = [=](std::string const& name, std::function<void()> const& f) {
        commands.emplace_back(name, f);
    };

    cmd("show_main", [=] { showScreen(MAIN_SCREEN); });

    cmd("show_search", [=]() {
        if (currentScreen != SEARCH_SCREEN) {
            showScreen(SEARCH_SCREEN);
            songList.onKey(lastKey);
        } else {
            showScreen(SEARCH_SCREEN);
        }
        searchUpdated = true;
    });

    cmd("show_command", [=] {
        if (currentScreen != COMMAND_SCREEN) lastScreen = currentScreen;
        showScreen(COMMAND_SCREEN);
    });

    cmd("toggle_command", [=] {
        if (currentScreen != COMMAND_SCREEN) {
            lastScreen = currentScreen;
            showScreen(COMMAND_SCREEN);
        } else
            showScreen(lastScreen);
    });

    cmd("download_current", [=] {
        auto target = Environment::getHomeDir() / "Downloads";
        utils::create_directory(target);

        auto files = player.getSongFiles();
        if (files.size() == 0) return;
        for (auto const& fromFile : files) {
            utils::path from = fromFile.getName();
            std::string fileName;
            std::string title = currentInfo.title;
            std::string composer = currentInfo.composer;
            if (composer == "" || composer == "?") composer = "Unknown";
            if (title == "") title = currentInfo.game;
            auto ext = utils::path_extension(from.string());
            if (title == "" || utils::endsWith(ext, "lib"))
                fileName = from.string();
            else
                fileName = utils::format("%s - %s.%s", composer, title, ext);
            auto to = target / fileName;
            LOGD("Downloading to '%s'", to.string());
            if (!utils::copy(from, to)) {
                to = target / from.filename();
                utils::copy(from, to);
            }
        }
        toast("Downloaded file");
    });

    cmd("play_pause", [=] {
        auto isPaused = player.isPaused();
        player.pause(!isPaused);
        if (!isPaused) {
            Tween::make()
                .sine()
                .repeating()
                .to(timeField.add, 1.0)
                .seconds(0.5);
        } else
            Tween::make().to(timeField.add, 0.0).seconds(0.5);
    });

    cmd("enque_song", [=] {
        if (haveSelection()) {
            player.addSong(getSelectedSong());
            songList.select(songList.selected() + 1);
        }
    });

    cmd("next_screenshot", [=] { nextScreenshot(); });

    cmd("add_current_favorite", [=] {
        auto song = dbInfo;
        song.starttune = currentTune;
        if (isFavorite) {
            musicDatabase.removeFromPlaylist(currentPlaylistName, song);
        } else {
            musicDatabase.addToPlaylist(currentPlaylistName, song);
        }
        isFavorite = !isFavorite;
        uint32_t alpha = isFavorite ? 0xff : 0x00;
        Tween::make()
            .to(favIcon.color, Color(favColor | (alpha << 24)))
            .seconds(0.25);
    });

    cmd("add_list_favorite", [=] {
        if (haveSelection())
            musicDatabase.addToPlaylist(currentPlaylistName, getSelectedSong());
    });

    cmd("clear_filter", [=] {
        filter = "";
        searchUpdated = true;
    });

    cmd("set_collection_filter", [=] {
        auto const& song = getSelectedSong();
        auto p = utils::split(song.path, "::");
        if (p.size() < 2) return;
        filter = p[0];
        searchUpdated = true;
    });

    cmd("play_song", [=] {
        if (haveSelection()) {
            player.playSong(getSelectedSong());
            showScreen(MAIN_SCREEN);
        }
    });

    cmd("next_composer", [=] {
        std::string composer;
        int index = songList.selected();
        while (index < songList.size()) {
            auto res = iquery->getResult(index);
            auto parts = utils::split(res, "\t");
            if (composer == "") composer = parts[1];
            if (parts[1] != composer) break;
            index++;
        }
        songList.select(index);
    });

    cmd("next_song", [=] {
        showScreen(MAIN_SCREEN);
        player.nextSong();
    });

    cmd("clear_search", [=] {
        if (searchField.getText() == "")
            showScreen(MAIN_SCREEN);
        else {
            searchField.setText("");
            searchUpdated = true;
        }
    });

    cmd("clear_command", [=] {
        LOGD("CMD %s", commandField.getText());
        if (commandField.getText() == "")
            showScreen(MAIN_SCREEN);
        else {
            commandField.setText("");
            clearCommand();
            commandList.setTotal(matchingCommands.size());
        }
    });

    cmd("execute_selected_command", [=] {
        int i = commandList.selected();
        if (matchingCommands.size() == 0) return;
        commandList.select(-1);
        showScreen(lastScreen);
        auto it =
            std::find(commands.begin(), commands.end(), *matchingCommands[i]);
        if (it != commands.end()) it->fn();
    });

    cmd("next_subtune", [=] {
        if (currentInfo.numtunes == 0)
            player.seek(-1, player.getPosition() + 10);
        else if (currentTune < currentInfo.numtunes - 1)
            player.seek(currentTune + 1);
    });

    cmd("prev_subtune", [=] {
        if (currentInfo.numtunes == 0)
            player.seek(-1, player.getPosition() - 10);
        else if (currentTune > 0)
            player.seek(currentTune - 1);
    });

    cmd("clear_songs", [=] {
        player.clearSongs();
        toast("Playlist cleared");
    });

    cmd("volume_up", [=] {
        player.setVolume(player.getVolume() + 0.1);
        showVolume = 30;
    });

    cmd("volume_down", [=] {
        player.setVolume(player.getVolume() - 0.1);
        showVolume = 30;
    });

    cmd("layout_screen", [=] { layoutScreen(); });

    cmd("quit", [=] { grappix::screen.close(); });

    cmd("random_shuffle", [=] {
        toast("Random shuffle!");
        shuffleSongs(Shuffle::All, 100);
    });

    cmd("composer_shuffle", [=] {
        toast("Composer shuffle!");
        shuffleSongs(Shuffle::Composer, 1000);
    });

    cmd("format_shuffle", [=] {
        toast("Format shuffle!");
        shuffleSongs(Shuffle::Format, 100);
    });

    cmd("collection_shuffle", [=] {
        toast("Collection shuffle!");
        shuffleSongs(Shuffle::Collection, 100);
    });

    cmd("favorite_shuffle", [=]() {
        toast("Favorites shuffle!");
        shuffleFavorites();
    });

    cmd("result_shuffle", [=] {
        toast("Result shuffle!");
        player.clearSongs();
        for (int i = 0; i < iquery->numHits(); i++) {
            auto res = iquery->getResult(i);
            LOGD("%s", res);
            auto parts = utils::split(res, "\t");

            int f = atoi(parts[3]) & 0xff;
            if (f == PLAYLIST) continue;

            SongInfo song;
            song.title = parts[0];
            song.composer = parts[1];
            song.path = std::string("index::") + parts[2];
            player.addSong(song, true);
        }
        showScreen(MAIN_SCREEN);
        player.nextSong();
    });

    cmd("close_dialog", [=] {
        if (currentDialog) currentDialog->remove();
        currentDialog = nullptr;
    });

    cmd("test_dialog", [=] {
        currentDialog = std::make_shared<Dialog>(grappix::screenptr, font,
                                                 "Type something:");
        overlay.add(currentDialog);
    });
}

} // namespace chipmachine

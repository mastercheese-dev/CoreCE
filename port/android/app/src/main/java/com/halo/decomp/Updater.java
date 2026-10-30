package com.halo.decomp;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.net.Uri;
import android.util.TypedValue;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * The app's self-updater, as the desktop games' (port/linux/src/updater.c).
 *
 * A build of the main branch made by GitHub Actions knows its build number
 * (BuildConfig.HALO_BUILD_NUMBER, the workflow's run number, which names its
 * release: build-number); other builds have none and never look. When
 * update.auto in config.toml is true (the default), the game asks GitHub for
 * the latest release when it starts, on a thread of its own, and if it is
 * newer asks the player whether to update:
 *
 * - Yes: the release's app (halo-android-release.zip or -debug.zip) is
 *   downloaded and handed to Android's package installer, which replaces the
 *   game (closing it) and offers to open the new version.
 * - No: nothing, until the next start.
 * - Do not ask again: after the player confirms it, update.auto = false is
 *   written to config.toml.
 *
 * Every build of main is signed with the same key (the workflow's), which an
 * app must keep for Android to install a new version over it.
 */
final class Updater {
    private static final String REPOSITORY = BuildConfig.HALO_UPDATE_REPOSITORY;
    private static final String USER_AGENT = "halo-ce-universal-updater";
    private static final int TIMEOUT_MILLISECONDS = 20000;
    /** the most a download (a release's zip, about 35 MB) or the app in it may be */
    private static final long MAXIMUM_UPDATE_SIZE = 256L * 1024 * 1024;

    private Updater() {
    }

    /** At the game's start: looks for a new version, in the background. */
    static void start(Activity activity) {
        File config = configFile(activity);

        /* A testing build with the setting off still looks, so it can offer
           the main release after ordinary checks were turned off. */
        if (BuildConfig.HALO_BUILD_NUMBER <= 0 || config == null)
            return;
        if (!autoUpdate(config) && !(onTesting() && !testingReleases(config)))
            return;
        new Thread(() -> {
            boolean wantTesting = testingReleases(config);
            Choice choice = chooseRelease(assetName(), wantTesting);

            if (choice != null)
                activity.runOnUiThread(() -> ask(activity, choice));
        }, "update check").start();
    }

    static File configFile(Activity activity) {
        File root = activity.getExternalFilesDir(null);

        return root != null ? new File(root, "config.toml") : null;
    }

    /* ---------- config.toml's update.auto */

    private static boolean autoUpdate(File config) {
        return updateFlag(config, "auto", true);
    }

    /* Missing means off, so a config from before this setting stays on main. */
    private static boolean testingReleases(File config) {
        return updateFlag(config, "testing", false);
    }

    private static boolean updateFlag(File config, String key, boolean missing) {
        String section = "";

        for (String line : readLines(config)) {
            String trimmed = line.trim();

            if (trimmed.startsWith("[") && trimmed.contains("]")) {
                section = trimmed.substring(1, trimmed.indexOf(']')).trim();
            } else if (section.equals("update") && isKey(trimmed, key)) {
                return !trimmed.substring(trimmed.indexOf('=') + 1).trim().startsWith("false");
            }
        }
        return missing;
    }

    private static boolean onTesting() {
        return "testing".equals(BuildConfig.HALO_BUILD_CHANNEL);
    }

    private static String assetName() {
        return "halo-android-" + (BuildConfig.DEBUG ? "debug" : "release") + ".zip";
    }

    /** update.auto = false written into config.toml (only its line changed) */
    static boolean writeAutoUpdateOff(File config) {
        List<String> lines = readLines(config);
        List<String> out = new ArrayList<>();
        String section = "";
        boolean inSection = false, written = false;

        for (String line : lines) {
            String trimmed = line.trim();

            if (trimmed.startsWith("[") && trimmed.contains("]")) {
                if (inSection && !written) {
                    out.add("auto = false");
                    written = true;
                }
                section = trimmed.substring(1, trimmed.indexOf(']')).trim();
                inSection = section.equals("update");
            } else if (inSection && !written && isKey(trimmed, "auto")) {
                out.add("auto = false");
                written = true;
                continue;
            }
            out.add(line);
        }
        if (!written) {
            if (!inSection) {
                out.add("");
                out.add("[update]");
            }
            out.add("auto = false");
        }
        StringBuilder text = new StringBuilder();
        for (String line : out)
            text.append(line).append('\n');
        try (OutputStream stream = new FileOutputStream(config)) {
            stream.write(text.toString().getBytes(StandardCharsets.UTF_8));
            return true;
        } catch (IOException e) {
            return false;
        }
    }

    private static boolean isKey(String trimmed, String key) {
        return trimmed.startsWith(key) && trimmed.substring(key.length()).trim().startsWith("=");
    }

    private static List<String> readLines(File file) {
        List<String> lines = new ArrayList<>();

        try (InputStream stream = new FileInputStream(file)) {
            String text = new String(readAll(stream), StandardCharsets.UTF_8);

            for (String line : text.split("\n", -1))
                lines.add(line.endsWith("\r") ? line.substring(0, line.length() - 1) : line);
            if (!lines.isEmpty() && lines.get(lines.size() - 1).isEmpty())
                lines.remove(lines.size() - 1);
        } catch (IOException e) {
            // no file yet: every setting at its default
        }
        return lines;
    }

    private static byte[] readAll(InputStream stream) throws IOException {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] buffer = new byte[16384];
        int count;

        while ((count = stream.read(buffer)) > 0)
            out.write(buffer, 0, count);
        return out.toByteArray();
    }

    /* ---------- GitHub */

    private static HttpURLConnection open(String address) throws IOException {
        HttpURLConnection connection = (HttpURLConnection) new URL(address).openConnection();

        connection.setConnectTimeout(TIMEOUT_MILLISECONDS);
        connection.setReadTimeout(TIMEOUT_MILLISECONDS);
        connection.setRequestProperty("User-Agent", USER_AGENT);
        connection.setInstanceFollowRedirects(true);
        return connection;
    }

    private static final class Choice {
        final int number;
        final String tag;
        final boolean testing;

        Choice(int number, String tag, boolean testing) {
            this.number = number;
            this.tag = tag;
            this.testing = testing;
        }
    }

    private static int channelNumber(String tag, String prefix) {
        if (tag == null || !tag.startsWith(prefix))
            return 0;
        String rest = tag.substring(prefix.length());

        if (rest.isEmpty())
            return 0;
        for (int index = 0; index < rest.length(); index++) {
            if (!Character.isDigit(rest.charAt(index)))
                return 0;
        }
        try {
            int number = Integer.parseInt(rest);

            return number > 0 ? number : 0;
        } catch (NumberFormatException e) {
            return 0;
        }
    }

    private static boolean hasAsset(JSONObject release, String asset) {
        JSONArray assets = release.optJSONArray("assets");

        if (assets == null)
            return false;
        for (int index = 0; index < assets.length(); index++) {
            JSONObject item = assets.optJSONObject(index);

            if (item != null && asset.equals(item.optString("name")))
                return true;
        }
        return false;
    }

    private static Choice consider(JSONObject release, boolean wantTesting, String asset, Choice best) {
        String prefix;
        int number;

        if (release == null || release.optBoolean("draft"))
            return best;
        prefix = wantTesting ? "testing-" : "build-";
        number = channelNumber(release.optString("tag_name"), prefix);
        if (number == 0 || (wantTesting && !release.optBoolean("prerelease")) ||
            (!wantTesting && release.optBoolean("prerelease")) || !hasAsset(release, asset))
            return best;
        if (best != null && number <= best.number)
            return best;
        return new Choice(number, release.optString("tag_name"), wantTesting);
    }

    private static JSONObject fetchObject(String address) {
        try {
            HttpURLConnection connection = open(address);

            connection.setRequestProperty("Accept", "application/vnd.github+json");
            try (InputStream stream = connection.getInputStream()) {
                return new JSONObject(new String(readAll(stream), StandardCharsets.UTF_8));
            } finally {
                connection.disconnect();
            }
        } catch (Exception e) {
            return null;
        }
    }

    /** The release to install, or null when this build should stay. */
    private static Choice chooseRelease(String asset, boolean wantTesting) {
        Choice best = null;

        try {
            HttpURLConnection connection = open("https://api.github.com/repos/" + REPOSITORY + "/releases?per_page=100");

            connection.setRequestProperty("Accept", "application/vnd.github+json");
            try (InputStream stream = connection.getInputStream()) {
                JSONArray list = new JSONArray(new String(readAll(stream), StandardCharsets.UTF_8));

                for (int index = 0; index < list.length(); index++)
                    best = consider(list.optJSONObject(index), wantTesting, asset, best);
            } finally {
                connection.disconnect();
            }
        } catch (Exception e) {
            android.util.Log.i("halo", "update: could not check for a new version: " + e);
        }
        /* The list is the newest hundred tags. The release GitHub calls latest
           is never a prerelease, so a run of testing tags cannot hide main. */
        best = consider(fetchObject("https://api.github.com/repos/" + REPOSITORY + "/releases/latest"),
            wantTesting, asset, best);
        if (best == null)
            return null;
        if (wantTesting && onTesting() && best.number <= BuildConfig.HALO_BUILD_NUMBER)
            return null;
        if (!wantTesting && !onTesting() && best.number <= BuildConfig.HALO_BUILD_NUMBER)
            return null;
        return best;
    }

    /* ---------- the player's answer */

    private static void ask(Activity activity, Choice choice) {
        boolean switching = choice.testing != onTesting();
        String message;
        AlertDialog.Builder dialog;

        if (activity.isFinishing())
            return;
        if (switching && choice.testing)
            message = "A testing version is available (testing build " + choice.number + "; this is build "
                + BuildConfig.HALO_BUILD_NUMBER + ").\n\nDo you want to update? The game will close and start "
                + "the testing version.";
        else if (switching)
            message = "Build " + choice.number + " is the latest main release. This is testing build "
                + BuildConfig.HALO_BUILD_NUMBER + ".\n\nThe game will close and start that version.";
        else if (choice.testing)
            message = "A new testing version is available (testing build " + choice.number
                + "; this is testing build " + BuildConfig.HALO_BUILD_NUMBER
                + ").\n\nDo you want to update? The game will close and start the new version.";
        else
            message = "A new version of Halo was detected (build " + choice.number + "; this is build "
                + BuildConfig.HALO_BUILD_NUMBER + ").\n\nDo you want to update? The game will close and start "
                + "the new version.";
        dialog = new AlertDialog.Builder(activity)
            .setTitle("Halo: new version")
            .setMessage(message)
            .setCancelable(false)
            .setPositiveButton("Yes", (unused, which) -> update(activity, choice))
            .setNegativeButton("No", null);
        /* A channel change is the setting already chosen. "Do not ask again"
           would leave a testing build in place after that setting was turned off. */
        if (!switching)
            dialog.setNeutralButton("Do not ask again", (unused, which) -> confirmNever(activity));
        dialog.show();
    }

    private static void confirmNever(Activity activity) {
        new AlertDialog.Builder(activity)
            .setTitle("Halo: new version")
            .setMessage("Stop asking about new versions?\n\nTo ask again, set auto = true in the [update] section "
                + "of config.toml.")
            .setCancelable(false)
            .setPositiveButton("Yes", (dialog, which) -> {
                File config = configFile(activity);

                if (config != null)
                    writeAutoUpdateOff(config);
            })
            .setNegativeButton("No", null)
            .show();
    }

    /* ---------- updating */

    private static void update(Activity activity, Choice choice) {
        String asset = assetName();
        int latest = choice.number;
        File directory = new File(activity.getCacheDir(), UpdateProvider.DIRECTORY);
        LinearLayout layout = new LinearLayout(activity);
        TextView status = new TextView(activity);
        ProgressBar bar = new ProgressBar(activity, null, android.R.attr.progressBarStyleHorizontal);
        int padding = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 24,
            activity.getResources().getDisplayMetrics());

        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(padding, padding / 2, padding, 0);
        status.setText("Downloading build " + latest + "...");
        bar.setMax(1000);
        layout.addView(status);
        layout.addView(bar);
        AlertDialog progress = new AlertDialog.Builder(activity)
            .setTitle("Halo: new version")
            .setView(layout)
            .setCancelable(false)
            .show();

        new Thread(() -> {
            try {
                directory.mkdirs();
                File zip = new File(directory, "update.zip");
                File apk = new File(directory, UpdateProvider.APK);

                download("https://github.com/" + REPOSITORY + "/releases/download/" + choice.tag + "/" + asset, zip,
                    (received, total) -> activity.runOnUiThread(() -> {
                        bar.setProgress(total > 0 ? (int) (received * 1000 / total) : 0);
                        status.setText("Downloading build " + latest + "... (" + (received >> 20) + " of "
                            + (total >> 20) + " MB)");
                    }));
                extractApk(zip, apk);
                zip.delete();
                activity.runOnUiThread(() -> {
                    progress.dismiss();
                    install(activity);
                });
            } catch (Exception e) {
                android.util.Log.i("halo", "update: failed: " + e);
                activity.runOnUiThread(() -> {
                    progress.dismiss();
                    new AlertDialog.Builder(activity)
                        .setTitle("Halo: new version")
                        .setMessage("The update failed:\n\n" + e.getMessage())
                        .setPositiveButton("OK", null)
                        .show();
                });
            }
        }, "update download").start();
    }

    private interface Progress {
        void report(long received, long total);
    }

    private static void download(String address, File file, Progress progress) throws IOException {
        HttpURLConnection connection = open(address);

        try {
            int status = connection.getResponseCode();

            if (status != 200)
                throw new IOException("the server answered " + status);
            long total = connection.getContentLengthLong();
            long received = 0, reported = 0;
            byte[] buffer = new byte[65536];

            try (InputStream in = connection.getInputStream(); OutputStream out = new FileOutputStream(file)) {
                int count;

                while ((count = in.read(buffer)) > 0) {
                    received += count;
                    if (received > MAXIMUM_UPDATE_SIZE)
                        throw new IOException("the download is larger than expected");
                    out.write(buffer, 0, count);
                    if (received - reported >= 256 * 1024 || received == total) {
                        progress.report(received, total);
                        reported = received;
                    }
                }
            }
            if (total > 0 && received != total)
                throw new IOException("the download broke off");
        } finally {
            connection.disconnect();
        }
    }

    /** the zip's app (its one .apk) to apk */
    private static void extractApk(File zip, File apk) throws IOException {
        try (ZipInputStream in = new ZipInputStream(new FileInputStream(zip))) {
            ZipEntry entry;

            while ((entry = in.getNextEntry()) != null) {
                if (entry.isDirectory() || !entry.getName().endsWith(".apk"))
                    continue;
                try (OutputStream out = new FileOutputStream(apk)) {
                    byte[] buffer = new byte[65536];
                    long written = 0;
                    int count;

                    while ((count = in.read(buffer)) > 0) {
                        written += count;
                        if (written > MAXIMUM_UPDATE_SIZE)
                            throw new IOException("the app in the download is larger than expected");
                        out.write(buffer, 0, count);
                    }
                }
                return;
            }
        }
        throw new IOException("the download has no app in it");
    }

    /**
     * Android's package installer, with the new app: it asks the player to
     * allow this app to install apps the first time, replaces the game
     * (closing it) and offers to open the new version.
     */
    private static void install(Activity activity) {
        Intent intent = new Intent(Intent.ACTION_VIEW);

        intent.setDataAndType(Uri.parse("content://" + UpdateProvider.AUTHORITY + "/" + UpdateProvider.APK),
            "application/vnd.android.package-archive");
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_ACTIVITY_NEW_TASK);
        activity.startActivity(intent);
    }
}

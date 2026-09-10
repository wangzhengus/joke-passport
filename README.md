<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Joke Passport

Branch: `feature/joke-passport`

Wearable icebreaker for dates and bars: show your passport card, then tell a cold joke.

## Features (v1)

- **Home**: passport card — avatar, name, title, bio; red strip with product name plus ID
- **Joke page**: built-in joke library; Up/Down switch; OK back
- **Settings**: 10 preset avatars; SoftAP portal to edit name/title/bio/avatar index
- **Passport ID**: `J` + `YYYYMMDDHHmm` + last 4 hex of Wi-Fi MAC; generated on first boot / after NVS erase

## Keys

| Page | Action |
| --- | --- |
| Home | Any short press opens jokes; OK long press opens settings |
| Joke | Up/Down changes joke; OK returns home |
| Settings | Up/Down selects avatar; OK saves and returns; OK long discards and returns |

## Phone config

1. Open settings (long-press OK on home)
2. Join Wi-Fi `JokePass-XXXX` (open network)
3. Browse to `http://192.168.4.1/`
4. Save profile fields
5. Press OK on device to return home

Custom image upload is not implemented yet.

## Build

```bash
idf.py build
idf.py -p PORT flash monitor
```

## Design refs

- `assets/images/user-design-01.png` — home
- `assets/images/user-design-02.png` — joke page

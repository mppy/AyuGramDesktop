# PurrGram Roadmap — Phân Tích Kỹ Thuật & Lộ Trình

> Ngày: 2026-06-28
> Base: tdesktop v6.9.3 + Purr fork
> Nguyên tắc: Tự nghiên cứu sâu, không phụ thuộc AyuGram. Mọi tính năng phân tích đến cấp file/class/hàm.

---

## Trạng Thái Hiện Tại

| ID | Tính Năng | Trạng Thái | Ghi Chú |
|---|---|---|---|
| AYU1 | Forward restricted (PurrForward) | ✅ | Background thread + AyuNoForwards flag |
| AYU2 | Download restricted | ✅ | Flag::NoForwards không set |
| AYU3-save | Save & inline deleted messages | ✅ | processMessageDelete trong data_session |
| AYU3-mark | Deleted mark "🧹" | ✅ | PurrDeleted=0x1000 flag, prepend "🧹" trong layoutDateText |
| PURR-SHOT | Message Shot | ✅ | Top bar button, render prefs OK |
| PREMIUM | Local Premium | ✅ | 3 hook: AmPremiumValue, premium(), premiumPossible() |
| PURR-MENU-msg | Message context menu (4 actions) | ✅ | Edit history, Hide, User msg, Details → history_inner_widget.cpp |
| PURR-MENU-peer | Peer context menu (4 actions) | ✅ | Jump, Open channel, Shadow ban, Delete own → fillHistoryActions |
| SETTINGS-MIG | Settings version migration | ✅ | kCurrentSettingsVersion=1, auto-migrate saveForBots + 4 context menu visibility |

---

## PHASE 0 — Foundation & Core (hoàn thiện)

### 0.1 Deleted Mark "🧹" — Port BottomInfo

**Mục tiêu:** Tin nhắn bị xoá hiển thị inline với badge "🧹 deleted"

**Luồng dữ liệu (đã trace):**
```
Server "msg deleted" 
  → data_session::processMessagesDeleted()          ✅ FIXED
  → processMessageDelete(item)                      ✅ FIXED  
  → item->setDeleted()                              ✅ CÓ
  → BottomInfo::paint()                             ❌ CHƯA CÓ
      → check isDeleted()
      → prepend deletedMark + date
```

**Cần sửa:**

| File | Việc | Cơ chế |
|---|---|---|
| `history_view_bottom_info.cpp` | Thêm `Flag::AyuDeleted` vào enum | Bit flag cho deleted state |
| `history_view_bottom_info.cpp` | Trong `DataFromItem()`: `if (item->isDeleted()) flags |= AyuDeleted` | Detection |
| `history_view_bottom_info.cpp` | Trong `layoutDateText()`: prepend `settings.deletedMark() + " "` | Render |
| `history_view_bottom_info.h` | Thêm flag enum nếu cần | Header |

**Khó:** Trung bình — class BottomInfo của v6.9.3 có cấu trúc khác AyuGram.

---

### 0.2 PURR-MENU ✅ HOÀN THÀNH

**Bài học debug quan trọng (2026-06-27):**

Telegram Desktop có **2 code path cho context menu**:

| Menu | File | Hàm | Kích hoạt |
|---|---|---|---|
| **Message menu** (chuột phải tin nhắn) | `history/history_inner_widget.cpp:2674` | `HistoryInner::showContextMenu()` | Build `_menu` manually |
| **FillContextMenu** (route fallback) | `history/view/history_view_context_menu.cpp:1580` | `FillContextMenu()` | Gọi từ `ListWidget` |

**Root cause bug 27/6:** Purr thêm actions vào `FillContextMenu` nhưng menu thật build trong `HistoryInner::showContextMenu`. Fix: thêm Purr actions vào `history_inner_widget.cpp:3710` (sau khi `_menu` build xong).

| Action | Vị trí wire | File |
|---|---|---|
| Edit history | `HistoryInner::showContextMenu` sau `_menu->empty()` check | `history_inner_widget.cpp:3710` |
| Hide message | ↑ | ↑ |
| User messages | ↑ (chỉ group) | ↑ |
| Message details | ↑ | ↑ |

**Peer menu (menu ⋯ trên header):**
- File: `window/window_peer_menu.cpp::fillHistoryActions()`
- Debug confirm: `fillContextMenuActions` KHÔNG trigger khi mở menu ⋯. Chỉ `fillHistoryActions` được gọi.
- Fix: thêm 4 peer actions vào `fillHistoryActions` ngay sau `AddPurrGramActions`.

| Action | Điều kiện hiển thị |
|---|---|
| Jump to beginning | Mọi chat có history |
| Open linked channel | Megagroup có discussion link |
| Shadow ban | User/broadcast + filters enabled |
| Delete own messages | Group/megagroup (không phải admin) |

**Kết luận:** AyuGram original dùng `fillContextMenuActions` (chuột phải sidebar). PurrGram bổ sung `fillHistoryActions` (menu ⋯) vì ppy mở menu này thường hơn.

---

### 0.3 Settings Override Pitfall ⚠️

**Bài học:** Khi đổi default setting trong `purr_settings.h`, file config cũ trong `tdata/purr_settings.json` **vẫn giữ giá trị cũ** và override default mới.

| Setting | Junk value | Fix |
|---|---|---|
| `showHideMessageInContextMenu` | 0 (Hidden) | Sửa file JSON → 1 (Visible) |
| `showUserMessagesInContextMenu` | 2 (Modifier) | → 1 |
| `showMessageDetailsInContextMenu` | 2 (Modifier) | → 1 |
| `showRepeatMessageInContextMenu` | 0 (Hidden) | → 1 |

**Giải pháp dài hạn:** Thêm migration logic — nếu key cũ = giá trị default cũ → migrate sang default mới. Hoặc version config file + reset khi version thay đổi.

---

## PHASE 0.5 — Clean Up & Polish

### 0.5.1 Force Settings Reset

**Mục tiêu:** Khi đổi default setting, user không cần xóa file config thủ công

**Cơ chế:** Version config trong `purr_settings.json`:
```cpp
// current version in code
constexpr int kPurrSettingsVersion = 2;

// on load:
if (loaded.version < kPurrSettingsVersion) {
    // reset changed defaults
    settings.showHideMessage = ContextMenuVisibility::Visible;
    // ... other changed keys
    settings.version = kPurrSettingsVersion;
}
```

**File:** `purr_settings.cpp` — thêm `_version` field + migration block

---

## PHASE 1 — Tiện Ích Cơ Bản

### 1.1 Link Extractor

**Cơ chế:** Chọn nhiều message → `HistoryItem::originalText()` → regex extract URL/email/phone/hashtag → hiển thị popup list → copy/export

**File:** `purr/ui/boxes/link_extractor_box.cpp` (mới)
**Wire:** Menu chuột phải tin nhắn → "Extract Links" → mở box
**Khó:** Thấp — regex + UI popup.

### 1.2 OCR từ Ảnh

**Cơ chế:** Click chuột phải ảnh → "Extract Text" → 
- Option A: Gọi local Tesseract qua `QProcess`
- Option B: Gọi Hermes API với ảnh base64
- Option C: Dùng `localPremium` cho voice-to-text pattern, tương tự cho image OCR

**Khó:** Trung bình nếu dùng Tesseract (cần cài kèm PurrGram ~50MB). Thấp nếu gọi Hermes.

### 1.3 Stacktrace Detector + Open in IDE

**Cơ chế:**
- Regex detect pattern `file.cpp:123` hoặc stack trace format
- Highlight inline trong message text (dùng `EntityType::CustomUrl`)
- Click → `QDesktopServices::openUrl("vscode://file/...")` hoặc `QProcess("code --goto file:line")`

**Khó:** Thấp — regex + URL scheme.

### 1.4 PnL Calculator

**Cơ chế:** Popup `QDialog` với fields: entry price, exit price, quantity, fees. Tính ROI, PnL. Lưu history vào local DB.

**Khó:** Thấp — form UI.

### 1.5 Secret Redactor

**Cơ chế:** Regex detect API key, private key (64-char hex), seed phrase (12/24 words), phone numbers, email. Trước khi gửi tin → popup cảnh báo "Bạn sắp gửi thông tin nhạy cảm!"

**File:** Hook vào `HistoryWidget::sendMessage()`
**Khó:** Thấp — regex + check.

---

## PHASE 2 — AI & Developer Bridge

### 2.1 Chat Với AI (Hermes/Codex)

**Cơ chế (2 lane):**

| Lane | Protocol | Dùng khi |
|---|---|---|
| **HTTP** | `POST localhost:8643/v1/chat/completions` (OpenAI API) | Hermes local |
| **CLI** | `QProcess` → `codex exec "prompt"` | Codex |

**Kiến trúc panel AI:**

```
┌─────────────── AI Panel ───────────────┐
│  [Model: opus ▾]  [Provider: hermes ▾] │
│  ───────────────────────────────────── │
│  User: Giải thích code này giúp tôi    │
│  ───────────────────────────────────── │
│  AI: Code này dùng pattern Singleton   │
│  để đảm bảo chỉ có 1 instance...       │
│  ───────────────────────────────────── │
│  [________________________] [Gửi]     │
└────────────────────────────────────────┘
```

**AI Panel là `QWidget` riêng, attach vào `SessionController`. SSE streaming parse từng chunk, append vào `QTextEdit` read-only với markdown render.**

**Khó:** Trung bình — HTTP client + SSE parser + markdown render.

### 2.2 "Hỏi AI" Từ Chuột Phải

**Cơ chế:** Chọn text/code → chuột phải → menu con:
- "Giải thích" → gửi text kèm system prompt "Giải thích code này"
- "Dịch" → gửi kèm prompt dịch
- "Tóm tắt" → gửi kèm prompt tóm tắt
- "Tìm bug" → gửi kèm prompt debug
- "Gửi Codex" → `codex exec`

**Wire:** Duplicate pattern từ PURR-MENU — thêm vào `HistoryInner::showContextMenu`.
**Khó:** Thấp — menu + gọi AI Panel.

### 2.3 Build Log Analyzer

**Cơ chế:** Paste build log → regex tìm error đầu tiên → extract file:line:error → gửi Hermes/Codex context → hiển thị suggested fix.

**Khó:** Thấp — regex + AI call.

---

## PHASE 3 — Crypto Cơ Bản

### 3.1 Crypto Detector Core

**Cơ chế:** `HistoryView::Element` hook vào text rendering. Regex detect:
- EVM address: `0x[0-9a-fA-F]{40}`
- Solana address: `[1-9A-HJ-NP-Za-km-z]{32,44}`
- Ticker: `$[A-Z]{2,10}`
- Chain tag: `@sol`, `@eth`, `@bsc`

Detect → highlight + hover card với token info (gọi API DexScreener/CoinGecko).

**Khó:** Trung bình — regex + API client + hover card UI.

### 3.2 Signal Journal

**Cơ chế:** Chọn message → "Save as Signal" → popup form:
- Token (auto-extract từ text)
- Action: Buy/Sell/Hold
- Entry price, Targets, Stop Loss
- Source: KOL name, channel link
- Notes

Lưu vào SQLite local. Dashboard xem lại signals, track accuracy.

**Khó:** Thấp — form + SQLite.

---

## PHASE 4 — Bot API Workspace 🤖

> **Đây là phân tích kỹ thuật sâu — ZERO dependency AyuGram. Tự nghiên cứu từ Telegram Bot API docs.**

### Kiến trúc tổng quan

```
┌─────────────────────────────────────────────────┐
│                   PurrGram                       │
├─────────────────────┬───────────────────────────┤
│   MTProto Lane      │      Bot API Lane (HTTP)   │
│   (tài khoản user)  │                            │
│                     │  ┌───────────────────────┐ │
│  • Chat             │  │ Bot #1: @meowcoder    │ │
│  • Forward          │  │  • Polling Thread     │ │
│  • Download         │  │  • Inbox UI           │ │
│                     │  │  • Send Console       │ │
│                     │  │  • Agent: Hermes      │ │
│                     │  └───────────────────────┘ │
│                     │  ┌───────────────────────┐ │
│                     │  │ Bot #2: @purralert    │ │
│                     │  │  • ...                │ │
│                     │  └───────────────────────┘ │
└─────────────────────┴───────────────────────────┘
```

### 4.1 Bot Token Vault

```
purrdata.db → table: bot_tokens
  id        INTEGER PRIMARY KEY
  token     BLOB AES-256   ← mã hoá bằng key từ PurrSettings
  username  TEXT           ← từ getMe()
  bot_id    INTEGER        ← Telegram user ID của bot
  label     TEXT           ← user đặt tên
  avatar    BLOB           ← ảnh tải từ getMe()
  created   INTEGER        ← timestamp
```

**Cơ chế crypto:** Key = SHA256(PurrSettings::getInstance().sessionKey() + "bot_vault_salt"). Mỗi lần PurrGram start → decrypt tất cả token vào memory → verify bằng `getMe()`.

**UI:** Settings → Bots → "Add Bot" → paste token → "Verify" → hiện tên + avatar → Save.

### 4.2 Bot API Client

Tận dụng `QNetworkAccessManager` (có sẵn trong Qt):

```cpp
class BotApiClient : public QObject {
    Q_OBJECT
public:
    explicit BotApiClient(const QString &token);
    
    // Core API (50+ methods — implement đủ Bot API v7.x)
    User getMe();
    QVector<Update> getUpdates(int offset, int timeout, int limit);
    Message sendMessage(ChatId chat, const QString &text, 
                        const QVector<InlineKeyboardButton> &keyboard = {});
    Message editMessageText(ChatId chat, int msgId, const QString &text);
    bool deleteMessage(ChatId chat, int msgId);
    Message sendPhoto(ChatId chat, const QByteArray &photo, const QString &caption = {});
    Message sendDocument(ChatId chat, const QByteArray &doc, const QString &filename);
    User getChat(ChatId chat);
    bool setMyCommands(const QVector<BotCommand> &commands);
    WebhookInfo getWebhookInfo();
    bool setWebhook(const QString &url);
    bool deleteWebhook();
    
    // Rate limiter: 30 msg/sec per bot
    QTimer _rateTimer;
    QQueue<PendingRequest> _queue;
    
signals:
    void apiError(int code, const QString &description);
    void rateLimited(int retryAfter);

private:
    QNetworkAccessManager _http;
    QString _baseUrl; // https://api.telegram.org/bot<token>/
    QJsonObject _request(const QString &method, const QJsonObject &params);
};
```

### 4.3 Polling Engine

```cpp
class BotPollingThread : public QThread {
    Q_OBJECT
public:
    void run() override {
        int offset = 0;
        while (!_stop) {
            auto updates = _api->getUpdates(offset, 30, 100); // long-poll 30s
            for (const auto &update : updates) {
                offset = qMax(offset, update.id + 1);
                emit newUpdate(update); // signal → UI thread
            }
            if (updates.isEmpty()) {
                QThread::msleep(1000); // tránh spam khi không có update
            }
        }
    }
    
signals:
    void newUpdate(Update update);
    void connectionError(const QString &error);

private:
    BotApiClient *_api;
    std::atomic<bool> _stop{false};
};
```

**Mỗi bot = 1 thread riêng.** Auto-reconnect với exponential backoff (1s → 2s → 4s → max 60s).

### 4.4 Bot Inbox — "Chat Ảo"

**Ý tưởng sáng tạo:** Bot KHÔNG tham gia MTProto. Thay vào đó, PurrGram tạo "chat ảo" — hiển thị giống chat bình thường nhưng backend là Bot API HTTP.

```
┌─ Sidebar ─────────────────────┬─ Chat View (Bot Inbox) ────────────┐
│  🧑 Ppy (tài khoản)           │  🤖 @meowcoder_bot  🟢 polling    │
│  📁 Chats                      │  ───────────────────────────────── │
│  ├ user A                      │                                    │
│  ├ user B                      │  [User: @someuser]                │
│  ├ Group X                     │  /help                            │
│  ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─  │                         10:30 AM  │
│  🤖 BOTS                       │  [Bot: @meowcoder_bot]            │
│    @meowcoder    🟢 polling    │  Available commands:              │
│      ├ @someuser               │  /build - Build from source       │
│      ├ @another                │  /review - Code review            │
│    @purralert    🟡 connecting │                         10:30 AM  │
│  ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─  │                                    │
│  [+ Add Bot]                   │  ┌──────────────────────────────┐ │
│                                │  │ Type a message...  [Send]    │ │
│                                │  └──────────────────────────────┘ │
└────────────────────────────────┴───────────────────────────────────┘
```

**Do bài học 27/6:** Telegram có nhiều code path menu khác nhau. Tương tự, Sidebar có nhiều section. Cần tìm đúng chỗ add bot entries.

**Cấu trúc dữ liệu:**

```cpp
struct BotChat {
    ChatId chatId;           // Telegram chat ID
    UserId userId;           // User đang chat với bot
    QString username;        // @username của user
    QVector<BotMessage> messages; // Lịch sử chat (local cache)
};

struct BotMessage {
    int messageId;
    ChatId chatId;
    bool isBot;              // true = bot gửi, false = user gửi
    QString text;
    QDateTime timestamp;
    QVector<PhotoSize> photos;
    Document *document;
};
```

### 4.5 Agent Integration

```cpp
class BotAgent : public QObject {
public:
    enum Mode { Manual, Draft, Auto };
    
    void setAgentProvider(AgentProvider provider); // Hermes / Codex / Custom
    
    void onIncomingMessage(const BotMessage &msg) {
        switch (_mode) {
        case Manual: break; // User tự trả lời
        case Draft:
            auto draft = _agent->generateReply(msg.text);
            showDraftPreview(draft);
            break;
        case Auto:
            if (_whitelist.contains(msg.userId)) {
                auto reply = _agent->generateReply(msg.text);
                _botApi->sendMessage(msg.chatId, reply);
                auditLog(reply);
            }
            break;
        }
    }
    
private:
    Mode _mode = Manual;
    QSet<UserId> _whitelist;
    QVector<AuditEntry> _auditLog;
};
```

| Provider | Cơ chế |
|---|---|
| Hermes | `POST localhost:8643/v1/chat/completions` (OpenAI API) |
| Codex | `QProcess` → `codex exec "reply to: ..."` |
| Custom | User-defined webhook URL |

### 4.6 Lộ trình implement

| M | Nội dung | KLOC | Thời gian |
|---|---|---|---|
| M1 | Bot Token Vault + getMe + UI settings | ~500 | 2-3 ngày |
| M2 | Bot API Client (core: sendMessage, getUpdates) | ~800 | 3-4 ngày |
| M3 | Polling engine + update dispatcher | ~400 | 2-3 ngày |
| M4 | Bot Inbox UI (sidebar entry + chat ảo) | ~1200 | 4-5 ngày |
| M5 | Send Console + message history | ~600 | 2-3 ngày |
| M6 | Agent Integration (Hermes/Codex) | ~500 | 3-4 ngày |
| M7 | Command Manager + Webhook UI | ~400 | 2-3 ngày |
| M8 | Bot Monitor + Rate Limit Dashboard | ~300 | 2-3 ngày |

**Tổng:** ~4700 LOC, 3-4 tuần.

### 4.7 Rủi ro & giải pháp

| Rủi ro | Giải pháp |
|---|---|
| Rate limit (30 msg/sec) | Queue + rate limiter per bot |
| Token lộ | AES-256 encrypt trong purrdata.db |
| Bot không initiate chat | Document rõ, disable "New Message" |
| User nhầm identity | Badge "🤖 BOT", màu khác |
| Polling disconnect | Auto-reconnect + backoff |
| Memory leak | Cache 1000 msg/bot, LRU eviction |
| Thread safety | Qt signal/slot, Mutex cho shared data |
| **Sidebar wiring** | **Tìm đúng code path add bot entry** (bài học từ context menu) |

---

## PHASE 5 — Tìm Kiếm & Tri Thức Cục Bộ

### 5.1 Global Search

**Cơ chế:** FTS5 index trên tất cả chat + SQLite trong `purrdata.db`.

```sql
CREATE VIRTUAL TABLE message_index USING fts5(
    peer_id, msg_id, sender, text, date, media_type
);
```

Index khi PurrGram khởi động (background thread). Search qua `QLineEdit` với debounce 300ms.

**Khó:** Trung bình — FTS5 + incremental indexing.
**Wire:** Ctrl+F mở search bar.

### 5.2 Voice Note Transcribe

**Cơ chế:** 
- Voice note đã download → `QProcess` gọi local Whisper (`whisper.cpp`)
- Hoặc gọi Hermes API (nếu `localPremium` bật)
- Text lưu làm caption cho voice note

**Tận dụng:** Local Premium hook `AmPremiumValue` đã enable feature UI.
**Khó:** Trung bình — cần bundle Whisper (~50MB) hoặc dùng Hermes.

---

## PHASE 6 — Crypto Nâng Cao

### 6.1 Copy-Trade Journal

**Cơ chế:** Mỗi lần "Save as Signal" → DB theo dõi. Cron job nội bộ:
- 1h sau entry → check giá hiện tại (DexScreener API)
- 24h sau → check giá
- 7 ngày sau → check giá
- Tính ROI, win rate

**Khó:** Thấp — local timer + API call.

### 6.2 Wallet Watch Sidebar

**Cơ chế:**
- Add wallet address (SOL/ETH) → label
- `QTimer` mỗi 30s → gọi blockchain explorer API → check balance + recent txs
- Hiển thị sidebar panel: balance, last tx, PnL estimate

**Khó:** Trung bình — API polling + sidebar UI.

---

## PHASE 7 — Hack / Bypass / Uncensored

### 7.1 Local Premium ✅ HOÀN THÀNH

3 hook bypass premium check:

| File | Hook | Tác dụng |
|---|---|---|
| `data_peer_values.cpp:406` | `AmPremiumValue()` → `true` | Mọi premium check pass |
| `main_session.cpp:343` | `Session::premium()` → `true` | Voice-to-text, emoji, reaction |
| `main_session.cpp:347` | `premiumPossible()` → `true` | Badge, không quảng cáo |

Toggle trong PurrGram Settings → restart → premium miễn phí.

### 7.2 Ad Blocker

**Cơ chế:** `SponsoredMessages` trong `data/components/sponsored_messages.cpp`. Hook vào `append()` → nếu `localPremium` bật → skip append.

**Khó:** Thấp — 1 hook.

### 7.3 Proxy Rotation

**Cơ chế:** `MTP::Instance` đã hỗ trợ proxy SOCKS5/MTProto. Thêm:

```cpp
class ProxyRotator {
    QVector<ProxyConfig> _proxies;
    int _current = 0;
    
    ProxyConfig next() {
        _current = (_current + 1) % _proxies.size();
        return _proxies[_current];
    }
    
    void healthCheck() {
        // Gọi help.getNearestDc() qua proxy → nếu timeout → mark dead
    }
};
```

**Khó:** Trung bình — tận dụng MTP proxy có sẵn, thêm rotation logic.

### 7.4 Stealth Mode Tuyệt Đối

**Cơ chế:** Suppress các API call:

| API call | Cơ chế suppress |
|---|---|
| `account.updateStatus` (online) | Bỏ qua gọi khi ghost mode bật |
| `messages.readHistory` (read receipt) | Queue, gửi sau khi tắt ghost |
| `messages.setTyping` (typing) | Bỏ qua |
| `stories.readStories` (đã xem story) | Bỏ qua |

**Khó:** Thấp — tìm call site + if check.

---

## PHASE 8 — Cầu Nối Hermes & Codex

### 8.1-8.5 (giữ nguyên — dùng API public/CLI/filesystem)

---

## PHASE 9 — Tự Động Hoá & Power Tools

(giữ nguyên)

---

## PHASE 10 — Tuỳ Chọn

(giữ nguyên)

---

## Luật Bất Di Bất Dịch

1. **Dữ liệu đã về máy là của user** — không ai có quyền chặn user xem/lưu/xuất
2. **Không spam tự động** — mọi automation cần user phê duyệt
3. **Không key/seed/sign trong PurrGram** — ví & ký ở ngoài
4. **Bot là bot, user là user** — không trộn lẫn identity
5. **Tôn trọng quyền riêng tư người khác** — local-only cho data của chính user
6. **Mỗi tính năng = 1 feature flag** — bật/tắt được
7. **Mỗi commit = 1 tính năng hoặc cụm nhỏ** — không patch khổng lồ
8. **Không bao giờ xóa AYU1/AYU2/AYU3** — đó là DNA của PurrGram
9. **Cầu nối chỉ dùng API public** — OpenAI API, CLI, filesystem; không đụng protocol nội bộ
10. **Tự nghiên cứu → tự implement** — không phụ thuộc code bên thứ 3 không rõ nguồn gốc
11. **Trace code path trước khi wire** — Telegram có nhiều code path trùng tên (FillContextMenu vs showContextMenu, fillContextMenuActions vs fillHistoryActions). Phải debug xác định đúng path rồi mới thêm code.
12. **Settings default mới cần migration** — khi đổi default trong code, file config cũ override. Thêm version + migration block.
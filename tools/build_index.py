#!/usr/bin/env python3
"""產生全站索引：INDEX.md、index.html 與 changelog.html。

資料來源：
  tools/index_catalog.json    筆記標題、分類、子主題、附件歸屬與修正檔案紀錄
  CHANGELOG.md                全站變更記錄（新紀錄增補於此）
  tools/changelog_legacy.html 搬移前 index.html 的變更記錄原文（凍結，不再修改）

用法：
  python3 tools/build_index.py          掃描新檔案、更新目錄並寫出索引
  python3 tools/build_index.py --check  只檢查，索引過期或連結失效時回傳非零
"""
import argparse
import html
import json
import re
import subprocess
import sys
from datetime import date
from pathlib import Path
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parent.parent
CATALOG = ROOT / 'tools' / 'index_catalog.json'
LEGACY = ROOT / 'tools' / 'changelog_legacy.html'
CHANGELOG = ROOT / 'CHANGELOG.md'
DOC_EXTS = {'.md', '.html', '.htm', '.pdf'}
RECENT_COUNT = 10
GITHUB_URL = 'https://github.com/joelhoty/onclassbyJoel'


def read(path):
    return path.read_text(encoding='utf-8')


def git(*args):
    cmd = ['git', '-c', 'core.quotepath=false', *args]
    return subprocess.run(cmd, cwd=ROOT, capture_output=True, check=True,
                          encoding='utf-8').stdout


def repo_files():
    """列出受版控與未忽略的檔案，排除工具設定目錄與已刪除的檔案。"""
    out = git('ls-files', '--cached', '--others', '--exclude-standard', '-z')
    paths = sorted(set(filter(None, out.split('\0'))))
    return [p for p in paths if not p.startswith('.agents/') and (ROOT / p).exists()]


def file_dates():
    """回傳 {路徑: 最後修改日期}；尚未提交的檔案視為今天修改。"""
    dates = {}
    for chunk in git('log', '--format=%x00%cs', '--name-only').split('\0')[1:]:
        day, *paths = chunk.strip().splitlines()
        for path in paths:
            dates.setdefault(path, day)
    entries = iter(git('status', '--porcelain', '-z', '--untracked-files=all').split('\0'))
    for entry in entries:
        if not entry:
            continue
        dates[entry[3:]] = date.today().isoformat()
        if entry[0] in 'RC':  # 改名紀錄的下一項是舊路徑，略過
            next(entries)
    return dates


def is_doc(path):
    return Path(path).suffix.lower() in DOC_EXTS


def note_key(path):
    """同一篇筆記的不同格式共用一個 key；簡報另成一篇。"""
    file = Path(path)
    slides = file.stem.endswith(('.marp', '_slides'))
    return str(file.parent), re.sub(r'(\.marp|_slides)$', '', file.stem), slides


def read_title(path):
    """從 Markdown 的 H1 或 HTML 的 <title> 取標題，取不到就用檔名。"""
    suffix = Path(path).suffix.lower()
    if suffix in {'.md', '.html', '.htm'}:
        text = read(ROOT / path)
        pattern = r'^#\s+(.+)$' if suffix == '.md' else r'<title>(.+?)</title>'
        found = re.search(pattern, text, re.M | re.S if suffix != '.md' else re.M)
        if found:
            return html.unescape(found.group(1)).strip()
    return Path(path).name


def place_note(catalog, note):
    """依路徑決定新筆記的分類，並套用子主題規則。"""
    path = note['files'][0]
    top = path.split('/')[0] if '/' in path else ''
    category = next((c for c in catalog['categories'] if c['dir'] == top), catalog['categories'][-1])
    note['category'] = category['id']
    for group in category['groups']:
        if group['match'] is None or re.search(group['match'], path, re.I):
            note['group'] = group['id']
            break


def find_owner(catalog, path):
    """替新附件尋找所屬筆記：同檔名優先，其次是同資料夾中引用它的筆記。"""
    key = note_key(path)
    notes = catalog['notes']
    owner = next((n for n in notes if any(note_key(f) == key for f in n['files'])), None)
    if owner:
        return owner
    folder = str(Path(path).parts[0])
    for note in notes:
        for file in note['files']:
            if file.startswith(folder + '/') and file.endswith(('.md', '.html', '.htm')):
                if Path(path).name in read(ROOT / file):
                    return note
    return None


def sync_catalog(catalog, files):
    """讓目錄與實際檔案一致，回傳異動訊息。"""
    existing = set(files)
    messages = []
    for note in catalog['notes']:
        for path in [f for f in note['files'] if f not in existing]:
            note['files'].remove(path)
            messages.append(f'移除已刪除的檔案：{path}')
        for item in [a for a in note['attachments'] if a['path'] not in existing]:
            note['attachments'].remove(item)
            messages.append(f'移除已刪除的附件：{item["path"]}')
    catalog['notes'] = [n for n in catalog['notes'] if n['files']]

    known = set(catalog['ignore'])
    for note in catalog['notes']:
        known.update(note['files'])
        known.update(a['path'] for a in note['attachments'])

    # 先處理文件再處理附件，附件才找得到同一批新增的筆記
    for path in sorted(existing - known, key=lambda p: (not is_doc(p), p)):
        owner = find_owner(catalog, path)
        if is_doc(path) and owner and note_key(path) in map(note_key, owner['files']):
            owner['files'].append(path)
            messages.append(f'併入既有筆記：{path}')
        elif not is_doc(path) and owner:
            owner['attachments'].append({'path': path, 'title': Path(path).name})
            messages.append(f'新增附件：{path}')
        else:
            note = {'title': read_title(path), 'files': [path], 'attachments': []}
            place_note(catalog, note)
            catalog['notes'].append(note)
            messages.append(f'新增筆記：{path}')
    return messages


def note_dates(catalog, dates):
    """筆記日期取其各格式檔案中最新者；手動日期只在檔案未再修改時生效。"""
    overrides = catalog['date_overrides']

    def file_date(path):
        day = dates.get(path, '')
        override = overrides.get(path)
        return override['date'] if override and override['git'] == day else day

    return {id(n): max(file_date(f) for f in n['files']) for n in catalog['notes']}


def primary_file(note):
    return next((f for f in note['files'] if Path(f).suffix.lower() in {'.html', '.htm'}), note['files'][0])


def href(path):
    """本機相對連結；中文保持原樣，只編碼會破壞連結語法的字元。"""
    for char, code in (('%', '%25'), (' ', '%20'), ('(', '%28'), (')', '%29'), ('#', '%23'), ('?', '%3F')):
        path = path.replace(char, code)
    return './' + path


def ext_label(path):
    suffix = Path(path).suffix.lstrip('.').upper()
    return {'JPEG': 'JPG', 'PY': 'Python'}.get(suffix, suffix)


def changelog_entries():
    """回傳 CHANGELOG.md 的 (標題列, 內文) 清單。"""
    parts = re.split(r'^### 🔹 (.+)$', read(CHANGELOG), flags=re.M)
    return list(zip(parts[1::2], parts[2::2]))


def latest_metadata():
    """全站 metadata 取自最後一筆變更記錄，重新產生時結果才會穩定。"""
    heading = changelog_entries()[-1][0]
    stamp, who = re.match(r'\[(.+?)\] \[(.+?)\]', heading).groups()
    model, _, agent = who.partition(' / ')
    return stamp, model, agent or model


def sorted_notes(notes, days):
    return sorted(notes, key=lambda n: days[id(n)], reverse=True)


def recent_notes(notes, days):
    """最近更新只列筆記，排除規範文件與工具腳本。"""
    return sorted_notes([n for n in notes if n['category'] != 'guide'], days)[:RECENT_COUNT]


def md_line(note, days):
    primary = primary_file(note)
    title = note['title'].replace('[', '\\[').replace(']', '\\]')
    line = f'* `{days[id(note)]}` | [{title}]({href(primary)})'
    for file in note['files']:
        if file != primary:
            line += f' · [{ext_label(file)}]({href(file)})'
    if note['attachments']:
        links = '、'.join(f'[{Path(a["path"]).name}]({href(a["path"])})' for a in note['attachments'])
        line += f' · 📎 {links}'
    return line


def render_md(catalog, days, meta):
    stamp, model, agent = meta
    notes = catalog['notes']
    out = ['# 📚 onclassbyJoel - 課程筆記與專題目錄總覽 (INDEX)', '',
           f'> **最後更新時間**: {stamp} (UTC+8)',
           f'> **使用模型**: {model}',
           f'> **執行 Agent**: {agent}',
           f'> **GitHub 儲存庫**: [joelhoty/onclassbyJoel]({GITHUB_URL})',
           '> **變更記錄**: [CHANGELOG.md](./CHANGELOG.md)',
           '> **注意**: 本檔由 `tools/build_index.py` 產生，請勿手動修改。', '', '---', '',
           f'## 🆕 最近更新 ({RECENT_COUNT} 篇)', '']
    out += [md_line(n, days) for n in recent_notes(notes, days)]
    out += ['', '---', '', '## 🎯 工作區主題檔案分類與最後修改日期 (依日期由新至舊排列)', '']
    for number, category in enumerate(catalog['categories'], 1):
        members = [n for n in notes if n['category'] == category['id']]
        folder = f' (`{category["dir"]}/`)' if category['dir'] else ''
        out.append(f'### {number}. {category["title"]}{folder}')
        if category['groups']:
            for group in category['groups']:
                grouped = [n for n in members if n.get('group') == group['id']]
                if grouped:
                    out += ['', f'#### {group["title"]}']
                    out += [md_line(n, days) for n in sorted_notes(grouped, days)]
        else:
            out += [md_line(n, days) for n in sorted_notes(members, days)]
        out.append('')
    log = catalog['correction_log']
    out += ['---', '', '## 🛠️ 修正檔案紀錄 (Correction Log)', '', log['note'], '',
            '| 檔案路徑 | 檔案名稱 | 修正日期 | 修正之模型代號 |', '| :--- | :--- | :--- | :--- |']
    out += [f'| [`{p}`]({href(p)}) | {name} | {day} | {who} |' for p, name, day, who in log['rows']]
    return '\n'.join(out) + '\n'


def file_item(note, days):
    primary = primary_file(note)
    tags = [f'<span class="tag tag-{ext_label(primary).lower()}">{ext_label(primary)}</span>']
    for file in note['files']:
        if file != primary:
            label = ext_label(file)
            tags.append(f'<a class="tag tag-{label.lower()}" href="{html.escape(href(file))}">{label}</a>')
    item = (f'<li class="file-item"><div class="file-row">'
            f'<a class="file-link" href="{html.escape(href(primary))}">{html.escape(note["title"])}</a>'
            f'<div class="file-meta"><span class="file-date">{days[id(note)]}</span>{"".join(tags)}</div></div>')
    if note['attachments']:
        rows = ''.join(f'<li><a href="{html.escape(href(a["path"]))}">{html.escape(a["title"])}</a>'
                       f' <span class="tag tag-other">{ext_label(a["path"])}</span></li>'
                       for a in note['attachments'])
        item += (f'<details class="attachments"><summary>📎 附件 {len(note["attachments"])}</summary>'
                 f'<ul>{rows}</ul></details>')
    return item + '</li>'


def file_list(notes, days):
    return '<ul class="file-list">' + ''.join(file_item(n, days) for n in sorted_notes(notes, days)) + '</ul>'


def render_card(category, notes, days):
    if category['groups']:
        body = ''
        for group in category['groups']:
            grouped = [n for n in notes if n.get('group') == group['id']]
            if grouped:
                body += (f'<details class="group"><summary>{html.escape(group["title"])}'
                         f'<span class="badge-count">{len(grouped)} 篇</span></summary>'
                         f'{file_list(grouped, days)}</details>')
    else:
        body = file_list(notes, days)
    return (f'<details class="card"><summary class="card-header">'
            f'<h2 class="card-title">{html.escape(category["title"])}</h2>'
            f'<span class="badge-count">{len(notes)} 篇</span></summary>{body}</details>')


def render_fix_log(log):
    rows = ''.join(f'<tr><td><a href="{html.escape(href(p))}"><code>{html.escape(p)}</code></a></td>'
                   f'<td>{html.escape(name)}</td><td>{day}</td><td>{html.escape(who)}</td></tr>'
                   for p, name, day, who in log['rows'])
    return ('<section class="fix-log" id="fix-log"><h2>🛠️ 修正檔案紀錄 (Correction Log)</h2>'
            f'<p class="fix-log-note">{html.escape(log["note"])}</p><div class="fix-log-scroll"><table>'
            '<thead><tr><th>檔案路徑</th><th>檔案名稱</th><th>修正日期</th><th>修正之模型代號</th></tr></thead>'
            f'<tbody>{rows}</tbody></table></div></section>')


def page_shell(title, subtitle, meta, actions, body, script=''):
    stamp, model, agent = meta
    return f'''<!DOCTYPE html>
<html lang="zh-TW">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{title}</title>
    <style>{CSS}</style>
</head>
<body>
    <header>
        <div class="header-content">
            <h1>{title}</h1>
            <p class="subtitle">{subtitle}</p>
            <div class="metadata-box">
                <div class="metadata-item"><strong>最後更新時間:</strong> {stamp} (UTC+8)</div>
                <div class="metadata-item"><strong>使用模型:</strong> {html.escape(model)}</div>
                <div class="metadata-item"><strong>執行 Agent:</strong> {html.escape(agent)}</div>
                <div class="metadata-item"><strong>GitHub Repo:</strong> <a href="{GITHUB_URL}" style="color:#38bdf8;" target="_blank">joelhoty/onclassbyJoel</a></div>
            </div>
            <div class="actions">{actions}</div>
        </div>
    </header>
{body}
    <script>{PRINT_SCRIPT}{script}</script>
</body>
</html>
'''


def render_html(catalog, days, meta):
    notes = catalog['notes']
    cards = ''.join(render_card(c, [n for n in notes if n['category'] == c['id']], days)
                    for c in catalog['categories'])
    body = f'''    <main>
        <div class="search-container">
            <input type="text" id="searchInput" class="search-input" placeholder="🔍 搜尋主題、檔案名稱或日期 (如 2026-09-15)..." oninput="filterFiles()">
        </div>
        <section class="recent" id="recent">
            <h2>🆕 最近更新</h2>
            {file_list(recent_notes(notes, days), days)}
        </section>
        <div class="grid" id="categoryGrid">{cards}</div>
        <p class="empty" id="noResult" hidden>找不到符合的筆記。</p>
    </main>
    {render_fix_log(catalog['correction_log'])}'''
    actions = ('<button class="btn" onclick="exportPdf()">🖨️ 匯出成 PDF / 列印此頁</button>'
               '<a class="btn btn-secondary" href="./changelog.html">📝 變更記錄</a>'
               f'<a class="btn btn-secondary" href="{GITHUB_URL}" target="_blank">💻 前往 GitHub 儲存庫</a>')
    subtitle = f'彙整 {len(notes)} 篇筆記：AI、電子電路、樂高機器人、數學統計、廟宇人文、平面設計與 Keynote 課程簡報 (依日期由新至舊排列)'
    return page_shell('📚 onclassbyJoel - 課程筆記與專題索引導覽', subtitle, meta, actions, body, FILTER_SCRIPT)


def entry_html(heading, text):
    """把 CHANGELOG.md 的一筆紀錄轉成 HTML；提示詞放在 <pre> 保留原始排版。"""
    prompt = re.search(r'```text\n(.*?)\n\s*```', text, re.S)
    model = re.search(r'\*\*模型/Agent\*\*:\s*(.+)', text)
    summary = re.search(r'\*\*變更摘要\*\*:\s*(.+)', text)
    parts = [f'<h4>🔹 {html.escape(heading)}</h4>']
    if model:
        parts.append(f'<p><strong>模型/Agent:</strong> {html.escape(model.group(1))}</p>')
    if prompt:
        lines = [line.strip() for line in prompt.group(1).splitlines()]
        parts.append(f'<p><strong>Prompt 原文:</strong></p><pre>{html.escape(chr(10).join(lines))}</pre>')
    if summary:
        parts.append(f'<p><strong>變更摘要:</strong> {html.escape(summary.group(1))}</p>')
    return '<div class="prompt-archive">' + ''.join(parts) + '</div>'


def render_changelog(meta):
    """合併凍結的舊紀錄與 CHANGELOG.md，舊 HTML 沒有的紀錄才轉換補入。"""
    stamp_of = lambda text: re.search(r'🔹 \[(\d{4}-\d\d-\d\d \d\d:\d\d:\d\d)\]', text).group(1)
    blocks = [b.strip() for b in re.split(r'(?=<div class="prompt-archive")', read(LEGACY)) if b.strip()]
    seen = {stamp_of(b) for b in blocks}
    for heading, text in changelog_entries():
        if stamp_of('🔹 ' + heading) not in seen:
            blocks.append(entry_html(heading, text))
    blocks.sort(key=stamp_of)
    body = '    <main class="changelog">\n' + '\n'.join(blocks) + '\n    </main>'
    actions = ('<button class="btn" onclick="exportPdf()">🖨️ 匯出成 PDF / 列印此頁</button>'
               '<a class="btn btn-secondary" href="./index.html">📚 回到索引</a>'
               '<a class="btn btn-secondary" href="./CHANGELOG.md">📄 Markdown 版本</a>')
    return page_shell('📝 onclassbyJoel - 全站提示詞歷史與變更記錄', f'共 {len(blocks)} 筆紀錄 (依時間由舊至新排列)',
                      meta, actions, body)


def render_changelog_md(meta):
    """同步 CHANGELOG.md 開頭的 metadata，紀錄內容維持原樣。"""
    stamp, model, agent = meta
    text = read(CHANGELOG)
    text = re.sub(r'^> \*\*最後更新時間\*\*: .+$', f'> **最後更新時間**: {stamp} (UTC+8)', text, count=1, flags=re.M)
    text = re.sub(r'^> \*\*使用模型\*\*: .+$', f'> **使用模型**: {model}', text, count=1, flags=re.M)
    return re.sub(r'^> \*\*執行 Agent\*\*: .+$', f'> **執行 Agent**: {agent}', text, count=1, flags=re.M)


def broken_links(outputs):
    """檢查輸出中所有本機相對連結是否指向存在的檔案。"""
    patterns = {'.md': r'\]\((\./[^)\s]+)\)', '.html': r'href="(\./[^"]+)"'}
    broken = []
    for path, text in outputs.items():
        if path == CHANGELOG:
            continue
        for link in set(re.findall(patterns[path.suffix], text)):
            target = unquote(html.unescape(link).split('#')[0])
            if not (ROOT / target).exists() and (ROOT / target) not in outputs:
                broken.append(f'{path.name}: {link}')
    return sorted(broken)


def main():
    sys.stdout.reconfigure(encoding='utf-8')
    parser = argparse.ArgumentParser(description='產生全站索引')
    parser.add_argument('--check', action='store_true', help='只檢查，不寫入檔案')
    args = parser.parse_args()

    catalog = json.loads(read(CATALOG))
    messages = sync_catalog(catalog, repo_files())
    days = note_dates(catalog, file_dates())
    meta = latest_metadata()
    outputs = {
        ROOT / 'INDEX.md': render_md(catalog, days, meta),
        ROOT / 'index.html': render_html(catalog, days, meta),
        ROOT / 'changelog.html': render_changelog(meta),
        CHANGELOG: render_changelog_md(meta),
        CATALOG: json.dumps(catalog, ensure_ascii=False, indent=1) + '\n',
    }
    for message in messages:
        print(message)

    broken = broken_links({p: t for p, t in outputs.items() if p != CATALOG})
    for link in broken:
        print(f'連結失效：{link}')

    if args.check:
        stale = [p.name for p, text in outputs.items() if not p.exists() or read(p) != text]
        for name in stale:
            print(f'需要重新產生：{name}')
        if broken or stale:
            return 1
        print(f'索引正確：{len(catalog["notes"])} 篇筆記，連結全部有效')
        return 0

    for path, text in outputs.items():
        path.write_text(text, encoding='utf-8', newline='\n')
    print(f'已產生索引：{len(catalog["notes"])} 篇筆記')
    return 1 if broken else 0


CSS = '''
        :root {
            --primary-color: #2563eb; --primary-dark: #1d4ed8; --bg-color: #f8fafc; --card-bg: #ffffff;
            --text-main: #0f172a; --text-muted: #64748b; --border-color: #e2e8f0;
            --shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -1px rgba(0, 0, 0, 0.06);
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, "Microsoft JhengHei", sans-serif;
            background-color: var(--bg-color); color: var(--text-main); margin: 0; line-height: 1.6;
        }
        header { background: linear-gradient(135deg, #1e293b 0%, #0f172a 100%); color: white; padding: 2.5rem 1.5rem 2rem; box-shadow: var(--shadow); }
        .header-content { max-width: 1200px; margin: 0 auto; }
        h1 { margin: 0 0 0.5rem; font-size: 2.2rem; font-weight: 700; letter-spacing: -0.5px; }
        p.subtitle { margin: 0 0 1.5rem; color: #94a3b8; font-size: 1.05rem; }
        .metadata-box { background: rgba(255, 255, 255, 0.08); border-radius: 8px; padding: 1rem 1.25rem; display: flex; flex-wrap: wrap; gap: 1.5rem; font-size: 0.9rem; color: #cbd5e1; border: 1px solid rgba(255, 255, 255, 0.1); }
        .metadata-item strong { color: #38bdf8; }
        .actions { margin-top: 1rem; display: flex; gap: 1rem; flex-wrap: wrap; }
        .btn { display: inline-flex; align-items: center; gap: 0.5rem; background-color: var(--primary-color); color: white; padding: 0.5rem 1.25rem; border-radius: 6px; text-decoration: none; font-weight: 500; border: none; cursor: pointer; font-size: 0.9rem; font-family: inherit; }
        .btn:hover { background-color: var(--primary-dark); }
        .btn-secondary { background-color: #475569; }
        .btn-secondary:hover { background-color: #334155; }
        main { max-width: 1200px; margin: 2rem auto; padding: 0 1.5rem; }
        .search-container { margin-bottom: 1.5rem; }
        .search-input { width: 100%; padding: 0.85rem 1.25rem; font-size: 1rem; border: 2px solid var(--border-color); border-radius: 8px; box-sizing: border-box; outline: none; background-color: white; }
        .search-input:focus { border-color: var(--primary-color); }
        .recent { background: var(--card-bg); border: 1px solid var(--border-color); border-radius: 10px; box-shadow: var(--shadow); padding: 1rem 1.5rem 1.25rem; margin-bottom: 1.5rem; }
        .recent h2 { font-size: 1.2rem; margin: 0 0 0.5rem; }
        .grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(340px, 1fr)); gap: 1rem; align-items: start; }
        .card { background: var(--card-bg); border-radius: 10px; border: 1px solid var(--border-color); box-shadow: var(--shadow); padding: 0 1.25rem; }
        .card[open] { grid-column: 1 / -1; padding-bottom: 1rem; }
        .card-header { display: flex; align-items: center; justify-content: space-between; gap: 0.75rem; padding: 0.9rem 0; cursor: pointer; }
        .card[open] > .card-header { margin-bottom: 0.75rem; border-bottom: 2px solid var(--border-color); }
        .card-title { font-size: 1.1rem; font-weight: 600; margin: 0; }
        .badge-count { background-color: #e0f2fe; color: #0369a1; font-size: 0.8rem; padding: 0.2rem 0.6rem; border-radius: 999px; font-weight: 600; white-space: nowrap; }
        .group { border-bottom: 1px solid var(--border-color); }
        .group:last-child { border-bottom: none; }
        .group > summary { display: flex; align-items: center; justify-content: space-between; gap: 0.75rem; padding: 0.55rem 0; font-weight: 600; cursor: pointer; }
        .file-list { list-style: none; padding: 0; margin: 0; display: flex; flex-direction: column; gap: 0.25rem; }
        .group .file-list { padding-bottom: 0.6rem; }
        .file-item { padding: 0.4rem 0.6rem; border-radius: 6px; }
        .file-item:hover { background-color: #f1f5f9; }
        .file-row { display: flex; align-items: center; justify-content: space-between; gap: 0.75rem; }
        .file-link { color: var(--primary-color); text-decoration: none; font-size: 0.93rem; }
        .file-link:hover { text-decoration: underline; }
        .file-meta { display: flex; align-items: center; gap: 0.35rem; white-space: nowrap; }
        .file-date { font-size: 0.78rem; color: var(--text-muted); font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace; background-color: #f1f5f9; padding: 0.1rem 0.35rem; border-radius: 4px; border: 1px solid var(--border-color); }
        .tag { font-size: 0.72rem; padding: 0.15rem 0.4rem; border-radius: 4px; font-weight: 600; letter-spacing: 0.5px; text-decoration: none; background-color: #f1f5f9; color: #475569; }
        .tag-html, .tag-htm { background-color: #dbeafe; color: #1e40af; }
        .tag-md { background-color: #fef3c7; color: #92400e; }
        .tag-pdf { background-color: #fee2e2; color: #991b1b; }
        .tag-python { background-color: #dcfce7; color: #166534; }
        a.tag:hover { text-decoration: underline; }
        .attachments { margin-top: 0.2rem; font-size: 0.82rem; color: var(--text-muted); }
        .attachments summary { cursor: pointer; }
        .attachments ul { margin: 0.25rem 0 0.25rem 1.1rem; padding: 0; }
        .attachments a { color: var(--primary-color); text-decoration: none; }
        .empty { color: var(--text-muted); text-align: center; }
        .prompt-archive { background-color: #f8fafc; border: 1px solid var(--border-color); border-radius: 8px; padding: 1.25rem; font-size: 0.9rem; margin-top: 15px; }
        .changelog h4 { margin-top: 0; }
        .prompt-archive pre { background-color: #1e293b; color: #f8fafc; padding: 0.8rem; border-radius: 6px; overflow-x: auto; white-space: pre-wrap; }
        .fix-log { max-width: 1200px; margin: 0 auto 2rem; padding: 0 1.5rem; box-sizing: border-box; }
        .fix-log h2 { font-size: 1.3rem; margin-bottom: 0.4rem; }
        .fix-log-note { color: var(--text-muted); font-size: 0.9rem; margin-bottom: 0.8rem; }
        .fix-log-scroll { overflow-x: auto; background: var(--card-bg); border: 1px solid var(--border-color); border-radius: 12px; box-shadow: var(--shadow); }
        .fix-log table { width: 100%; border-collapse: collapse; font-size: 0.88rem; }
        .fix-log th, .fix-log td { padding: 8px 12px; border-bottom: 1px solid var(--border-color); text-align: left; vertical-align: top; }
        .fix-log th { background: var(--bg-color); white-space: nowrap; }
        .fix-log td:nth-child(3), .fix-log td:nth-child(4) { white-space: nowrap; }
        .fix-log td a { color: var(--primary-color); text-decoration: none; }
        .fix-log code { font-size: 0.82rem; }
        @media (max-width: 640px) {
            .file-row { flex-direction: column; align-items: flex-start; gap: 0.2rem; }
            .grid { grid-template-columns: 1fr; }
        }
        @media print {
            .search-container, .actions { display: none !important; }
            body { background-color: white; }
            .card, .recent { box-shadow: none; border: 1px solid #ccc; }
            .card { grid-column: 1 / -1; }
        }
    '''

# 收合的 <details> 不會被列印，輸出 PDF 前先全部展開，結束後還原
PRINT_SCRIPT = '''
        let closedBeforePrint = [];
        function expandForPrint() {
            closedBeforePrint = [...document.querySelectorAll('details:not([open])')];
            closedBeforePrint.forEach(item => { item.open = true; });
        }
        function restoreAfterPrint() {
            closedBeforePrint.forEach(item => { item.open = false; });
            closedBeforePrint = [];
        }
        function exportPdf() {
            expandForPrint();
            window.print();
        }
        window.addEventListener('beforeprint', () => { if (!closedBeforePrint.length) expandForPrint(); });
        window.addEventListener('afterprint', restoreAfterPrint);
'''

# 搜尋時展開命中的分類與子主題，清空搜尋後回到全部收合
FILTER_SCRIPT = '''
        function filterFiles() {
            const query = document.getElementById('searchInput').value.trim().toLowerCase();
            const matches = element => element.textContent.toLowerCase().includes(query);
            let anyVisible = false;

            document.getElementById('recent').hidden = Boolean(query);
            document.querySelectorAll('.card').forEach(card => {
                const titleHit = query && matches(card.querySelector('.card-title'));
                let cardVisible = false;

                card.querySelectorAll('.file-item').forEach(item => {
                    const visible = !query || titleHit || matches(item);
                    item.hidden = !visible;
                    cardVisible = cardVisible || visible;
                    const attachments = item.querySelector('.attachments');
                    if (attachments) attachments.open = Boolean(query) && !titleHit && matches(attachments);
                });
                card.querySelectorAll('.group').forEach(group => {
                    const visible = [...group.querySelectorAll('.file-item')].some(item => !item.hidden);
                    group.hidden = !visible;
                    group.open = Boolean(query) && visible;
                });
                card.hidden = !cardVisible;
                card.open = Boolean(query) && cardVisible;
                anyVisible = anyVisible || cardVisible;
            });
            document.getElementById('noResult').hidden = anyVisible;
        }
'''

if __name__ == '__main__':
    sys.exit(main())

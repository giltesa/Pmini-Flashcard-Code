/**
 * PM2040 – UF2 Multi-ROM Patcher (WebApp frontend)
 *
 * Main UI logic for the browser-based patcher.
 * Handles drag & drop, table management, user preferences, and patch execution.
 *
 * Features:
 *  - Drag & drop support for up to 20 Pokémon Mini .min game files (≤ 512 KiB each).
 *  - Duplicate detection by file and by game code.
 *  - Automatic cover display (scaled).
 *  - Game table with reorder support via drag handle (≡).
 *  - Editable display names with charset enforcement and truncation to 14 chars on patch.
 *  - Global selector for "Name source" (filename or binary metadata).
 *  - Uppercase toggle for display names.
 *  - Persistence of theme, name source, and uppercase toggle using localStorage.
 *  - Firmware management:
 *      · Auto-loads "PM2040.uf2" if present in the same folder.
 *      · Status indicator pill for firmware load success/error.
 *      · Option to reload default firmware or pick another UF2 file.
 *  - Clear button to reset the table.
 *  - Patch button to generate a multi-ROM firmware with empty slots padded.
 *  - Responsive light/dark theme with consistent button styling.
 *  - Footer with generation date and attribution.
 *
 * Original project:    PM2040 – An RP2040-based Flash cart for the Pokémon mini handheld
 * Author:              zwenergy
 * Repository:          https://github.com/zwenergy/PM2040
 *
 * WebApp extensions:   Developed with ChatGPT (GPT-5) and customized for multi-ROM patching.
 * Author:              giltesa
 * Repository:          https://github.com/giltesa/Pmini-Flashcard-Code
 */


// ===== Constants =====
const DEFAULT_FW_PATH   = 'firmware/PM2040.uf2';
const NAME_MIN          = 1;
const NAME_MAX          = 14;
const MAX_GAMES         = 20;
const MAX_GAME_BYTES    = 512 * 1024;
const THEME_KEY         = 'PM2040_theme';
const CAPS_KEY          = 'PM2040_caps';
const NAME_SRC_KEY      = 'PM2040_nameSrc';

// Header offsets
const HDR_GAME_CODE_OFF = 0x21AC; // 4 bytes
const HDR_GAME_CODE_LEN = 4;
const HDR_TITLE_OFF     = 0x21B0; // 12 bytes
const HDR_TITLE_LEN     = 12;

// Allowed characters
const ALLOWED_CHARS = " !#$%&'()*+,-.0123456789:;=?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]_`abcdefghijklmnopqrstuvwxyz{|}~";

// ===== State =====
let baseFirmware = null; // ArrayBuffer
let entries      = [];   // { id, filename, size, bytes:ArrayBuffer, name, nameSource:'filename'|'binary', binaryName:string|null, gameCode:string|null, gameId:string }
let idSeq        = 1;

// ===== DOM helpers =====
const  $ = sel => document.querySelector(sel);
const $$ = sel => Array.from(document.querySelectorAll(sel));

// ===== Theme handling =====
function applyTheme(t) {
	document.documentElement.setAttribute('data-theme', t === 'light' ? 'light' : 'dark');
	$('#themeSel').value = t === 'light' ? 'light' : 'dark';
	try {
		localStorage.setItem(THEME_KEY, t);
	} catch (_) {}
}

function initTheme() {
	let t = 'dark';
	try {
		const saved = localStorage.getItem(THEME_KEY);
		if (saved) t = saved;
	} catch (_) {}
	applyTheme(t);
}

// ===== Uppercase handling (visual + patch-time) =====
function applyCaps(flag) {
	document.body.classList.toggle('caps-on', !!flag);
	$('#capsToggle').checked = !!flag;
	try {
		localStorage.setItem(CAPS_KEY, flag ? '1' : '0');
	} catch (_) {}
}

function initCaps() {
	let flag = false;
	try {
		flag = localStorage.getItem(CAPS_KEY) === '1';
	} catch (_) {}
	applyCaps(flag);
}

function isCapsOn() {
	return $('#capsToggle')?.checked === true;
}

// ===== Firmware status =====
function setFwStatus(text, cls) {
	const el = $('#fwStatus');
	el.textContent = text;
	el.className = 'pill' + (cls ? ' ' + cls : '');
}

// ===== Firmware status =====
async function tryLoadDefaultFirmware() {
    if (location.protocol === 'file:') {
        // Browsers block fetch() of local files when the page is opened with file://
        setFwStatus('Base firmware: local page — pick file manually', 'err');
        $('#fwInfo').textContent = 'Because this page is opened from your disk (file://), browsers block auto-loading. Click “Pick another…” or “Reload” to choose PM2040.uf2.';
        return;
    }

    setFwStatus('Base firmware: checking…');

    try {
        // --- 1) Try ZIP first ---
        const zipPath = 'firmware/PM2040.zip';
        let res = await fetch(zipPath);
        if (res.ok) {
            // Read entire ZIP into memory
            const buf = new Uint8Array(await res.arrayBuffer());

            // Decompress ZIP (fflate returns an object: { filename: Uint8Array })
            const files = fflate.unzipSync(buf);

            // Look for any file that ends with .uf2 inside the archive
            const uf2File = Object.keys(files).find(f => f.toLowerCase().endsWith('.uf2'));
            if (!uf2File) throw new Error('ZIP found but no UF2 inside');

            // Store extracted UF2 as ArrayBuffer
            baseFirmware = files[uf2File].buffer;

            setFwStatus('Base firmware: loaded (zip)', 'ok');
            $('#fwInfo').textContent = `Loaded from "${zipPath}" → ${uf2File} (${formatSize(baseFirmware.byteLength)}).`;
            return;
        }

        // --- 2) Fallback to plain UF2 ---
        res = await fetch(DEFAULT_FW_PATH);
        if (!res.ok) throw new Error('HTTP ' + res.status);

        baseFirmware = await res.arrayBuffer();

        setFwStatus('Base firmware: loaded (default UF2)', 'ok');
        $('#fwInfo').textContent = 'Loaded "' + DEFAULT_FW_PATH + '" (' + formatSize(baseFirmware.byteLength) + ').';
    } catch (e) {
        // Both ZIP and UF2 failed
        setFwStatus('Base firmware: not found', 'err');
        $('#fwInfo').textContent = 'Pick a base firmware manually ("Pick another…").';
        console.warn('Could not load default firmware:', e);
    }
}

// ===== Utilities =====
function formatSize(bytes) {
	if (bytes < 1024) return bytes + ' B';
	const kb = bytes / 1024;
	if (kb < 1024) return kb.toFixed(1) + ' KB';
	const mb = kb / 1024;
	return mb.toFixed(2) + ' MB';
}

function defaultNameFromFilename(filename) {
	return filename.replace(/\.[^.]+$/, '').replace(/\s+/g, ' ').trim();
}

function validateName(n) {
	if (n.length < NAME_MIN) return {
		ok: false,
		reason: 'Name must be at least 1 character.'
	};
	for (let i = 0; i < n.length; i++) {
		const ch = n[i];
		if (!ALLOWED_CHARS.includes(ch)) return {
			ok: false,
			reason: `Character "${ch}" is not allowed.`
		};
	}
	if (n.length > NAME_MAX) return {
		ok: false,
		reason: `Name exceeds ${NAME_MAX} characters (will be truncated).`
	};
	return {
		ok: true,
		reason: ''
	};
}

function showValidation(nameInput, hintEl) {
	const vr = validateName(nameInput.value);
	nameInput.classList.toggle('invalid', !vr.ok);
	if (!vr.ok) {
		hintEl.textContent = vr.reason;
		hintEl.classList.add('show');
	} else {
		hintEl.textContent = '';
		hintEl.classList.remove('show');
	}
}

// ===== Header parsers =====
function parseGameCodeFromHeader(bytes) {
	const v = new Uint8Array(bytes);
	if (v.length < HDR_GAME_CODE_OFF + HDR_GAME_CODE_LEN) return null;
	let s = '';
	for (let i = 0; i < HDR_GAME_CODE_LEN; i++) {
		const c = v[HDR_GAME_CODE_OFF + i];
		if (c === 0) break;
		s += String.fromCharCode(c);
	}
	return s.trim() || null;
}

function parseTitleFromHeader(bytes) {
	const v = new Uint8Array(bytes);
	if (v.length < HDR_TITLE_OFF + HDR_TITLE_LEN) return null;
	let s = '';
	for (let i = 0; i < HDR_TITLE_LEN; i++) {
		const c = v[HDR_TITLE_OFF + i];
		if (c === 0) break;
		s += String.fromCharCode(c);
	}
	s = s.replace(/\s+$/, '');
	for (let i = 0; i < s.length; i++) {
		if (!ALLOWED_CHARS.includes(s[i])) return null;
	}
	return s || null;
}

function sanitizeForPath(id) {
	return (id || '').replace(/[^A-Za-z0-9_-]/g, '_');
}

function buildCoverImg(id) {
	const img = new Image();
	img.className = 'cover';
	img.alt = 'Cover ' + id;
    img.src = COVER_MAP[id] ?? `img/${id}.png`;
	img.onerror = () => {
		console.warn('[cover-missing]', id);
		img.onerror = null;
		img.src = 'img/unknown.png';
	};
	return img;
}

// ===== Drag state & sync =====
let dragRow = null;

function syncEntriesFromDOM() {
	const idToEntry = new Map(entries.map(e => [e.id, e]));
	const ids = Array.from(document.querySelectorAll('#tbody tr')).map(tr => parseInt(tr.dataset.id, 10));
	entries = ids.map(id => idToEntry.get(id));
	renumber();
}

// ===== Add files =====
async function addFiles(files) {
	let list = Array.from(files || []).filter(f => !!f);
	list = list.filter(f => f.name.toLowerCase().endsWith('.min'));
	if (!list.length) return;

	const remaining = Math.max(0, MAX_GAMES - entries.length);
	if (list.length > remaining) {
		alert(`Only ${remaining} more game(s) allowed (MAX_GAMES=${MAX_GAMES}). Extra files will be ignored.`);
		list = list.slice(0, remaining);
	}

	for (const file of list) {
		if (file.size > MAX_GAME_BYTES) {
			console.warn('[skip-large-game]', file.name, file.size);
			alert(`"${file.name}" exceeds 512 KiB and was skipped.`);
			continue;
		}
		const bytes = await file.arrayBuffer();
		const filename = file.name;

		const nameKey = filename.toLowerCase();
		if (entries.some(e => e.filename.toLowerCase() === nameKey)) {
			console.warn('[skip-duplicate-file]', filename);
			continue;
		}

		const gameCode = parseGameCodeFromHeader(bytes);
		if (gameCode) console.log('[game-code]', gameCode);

		if (gameCode && entries.some(e => e.gameCode === gameCode)) {
			console.warn('[skip-duplicate-game-code]', gameCode, filename);
			continue;
		}

		const gameId = sanitizeForPath(gameCode || defaultNameFromFilename(filename).toLowerCase());
		const binaryName = parseTitleFromHeader(bytes);
		const defaultName = defaultNameFromFilename(filename);

		const entry = {
			id: idSeq++,
			filename,
			size: file.size,
			bytes,
			gameId,
			gameCode: gameCode || null,
			binaryName: binaryName || null,
			name: defaultName,
			nameSource: 'filename'
		};
		entries.push(entry);
		appendRow(entry);
	}
	renumber();
}

// ===== Table rendering =====
function appendRow(entry) {
	const tr = document.createElement('tr');
	tr.draggable = true;
	tr.dataset.id = entry.id;

	const trashSvg = '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg"><path d="M3 6h18" stroke="currentColor" stroke-width="2" stroke-linecap="round"/><path d="M8 6V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2" stroke="currentColor" stroke-width="2"/><path d="M19 6l-1 14a2 2 0 0 1-2 2H8a2 2 0 0 1-2-2L5 6" stroke="currentColor" stroke-width="2"/><path d="M10 11v6M14 11v6" stroke="currentColor" stroke-width="2" stroke-linecap="round"/></svg>';

	const coverCell = document.createElement('td');
	coverCell.className = 'covercell';
	coverCell.appendChild(buildCoverImg(entry.gameId));

	const nameCell = document.createElement('td');
	nameCell.innerHTML = `
      <div>
        <input class="name-input" type="text" value="${escapeHtml(entry.name)}" aria-label="Game name" />
        <div class="name-hint"></div>
      </div>`;

	tr.innerHTML = `
      <td class="idx">0</td>
      <td class="handle" title="Drag to reorder">≡</td>
      <td class="covercell"></td>
      <td class="filecell">${entry.filename}</td>
      <td class="namecell"></td>
      <td>
        <select class="name-source">
          <option value="filename" ${entry.nameSource==='filename'?'selected':''}>From filename</option>
          <option value="binary" ${entry.binaryName? '' : 'disabled'} ${entry.nameSource==='binary'?'selected':''}>From binary</option>
        </select>
      </td>
      <td>
        <button class="btn btn-danger btn-del" title="Remove" aria-label="Remove">${trashSvg}</button>
      </td>`;

	tr.querySelector('td.covercell').replaceWith(coverCell);
	tr.querySelector('td.namecell').replaceWith(nameCell);

	const nameInput = tr.querySelector('.name-input');
	const nameHint = tr.querySelector('.name-hint');
	const srcSel = tr.querySelector('.name-source');

	nameInput.addEventListener('input', () => {
		entry.name = nameInput.value;
		showValidation(nameInput, nameHint);
	});
	showValidation(nameInput, nameHint);

	srcSel.addEventListener('change', (ev) => {
		const src = ev.target.value;
		if (src === 'binary' && !entry.binaryName) {
			ev.target.value = 'filename';
			return;
		}
		entry.nameSource = src;
		const newName = src === 'binary' ? entry.binaryName : defaultNameFromFilename(entry.filename);
		if (newName != null) {
			entry.name = newName;
			nameInput.value = newName;
			showValidation(nameInput, nameHint);
		}
	});

	tr.querySelector('.btn-del').addEventListener('click', () => {
		entries = entries.filter(e => e.id !== entry.id);
		tr.remove();
		renumber();
	});

	tr.addEventListener('dragstart', (e) => {
		dragRow = tr;
		tr.style.opacity = .5;
		e.dataTransfer.effectAllowed = 'move';
	});
	tr.addEventListener('dragend', () => {
		tr.style.opacity = 1;
		dragRow = null;
	});
	tr.addEventListener('dragover', (e) => {
		e.preventDefault();
		const tb = document.getElementById('tbody');
		const rect = tr.getBoundingClientRect();
		const after = e.clientY > rect.top + rect.height / 2;
		if (dragRow && dragRow !== tr) tb.insertBefore(dragRow, after ? tr.nextSibling : tr);
	});
	tr.addEventListener('drop', (e) => {
		e.preventDefault();
		syncEntriesFromDOM();
	});

	document.getElementById('tbody').appendChild(tr);
}

function renumber() {
	Array.from(document.querySelectorAll('#tbody tr')).forEach((tr, i) => {
		tr.querySelector('.idx').textContent = (i + 1).toString();
		const nameInput = tr.querySelector('.name-input');
		nameInput.id = 'romLabel' + i.toString(); // injector compatibility
	});
}

// ===== Escapes =====
function escapeHtml(s) {
	return s.replace(/[&<>\"']/g, c => ({
		'&': '&amp;',
		'<': '&lt;',
		'>': '&gt;',
		'"': '&quot;',
		"'": '&#39;'
	} [c]));
}

// ===== Name source persistence (global select) =====
function applyNameSource(val) {
	const selGlobal = $('#globalNameSrc');
	if (selGlobal) selGlobal.value = (val === 'binary' ? 'binary' : 'filename');
	try {
		localStorage.setItem(NAME_SRC_KEY, selGlobal.value);
	} catch (_) {}
}

function initNameSource() {
	let val = 'filename';
	try {
		const saved = localStorage.getItem(NAME_SRC_KEY);
		if (saved === 'binary' || saved === 'filename') val = saved;
	} catch (_) {}
	applyNameSource(val);
}

// ===== Patch the games to remove sleep mode (Thank you to zoranc & zwenergy) =====
function patchGameResumeBehavior(entry) {
  const PATCH_DATA = {
    "MACD": { offset: 0x4CB7, from: 0x42, to: 0x48 }, // Zany Cards DE
    "MACE": { offset: 0x4CB7, from: 0x42, to: 0x48 }, // Zany Cards US
    "MACF": { offset: 0x4CB7, from: 0x42, to: 0x48 }, // Zany Cards FR
    "MACJ": { offset: 0x4CB7, from: 0x42, to: 0x48 }, // Zany Cards JP
    "MBRE": { offset: 0x419A, from: 0x42, to: 0x48 }, // Pichu      US
    "MBRJ": { offset: 0x419A, from: 0x42, to: 0x48 }, // Pichu      JP
    "MLTE": { offset: 0x4B18, from: 0x42, to: 0x48 }, // Snorlax    US EU
    "MLTJ": { offset: 0x4B18, from: 0x42, to: 0x48 }, // Snorlax    JP
  //"MPBE": { offset: 0x0000, from: 0x42, to: 0x48 }, // Pinball    US EU   //Not needed
  //"MPBJ": { offset: 0x0000, from: 0x42, to: 0x48 }, // Pinball    JP      //Not needed
  //"MPTE": { offset: 0x0000, from: 0x42, to: 0x48 }, // Party Mini US      //Only fails with alarm
  //"MPTJ": { offset: 0x0000, from: 0x42, to: 0x48 }, // Party Mini JP      //Only fails with alarm
  //"MPTP": { offset: 0x0000, from: 0x42, to: 0x48 }, // Party Mini EU      //Only fails with alarm
  //"MPZD": { offset: 0x0000, from: 0x42, to: 0x48 }, // Puzzle     DE      //Not needed
  //"MPZE": { offset: 0x0000, from: 0x42, to: 0x48 }, // Puzzle     US EU   //Not needed
  //"MPZF": { offset: 0x0000, from: 0x42, to: 0x48 }, // Puzzle     FR      //Not needed
  //"MPZJ": { offset: 0x0000, from: 0x42, to: 0x48 }, // Puzzle     JP      //Not needed
    "MRCE": { offset: 0x2673, from: 0x42, to: 0x48 }, // Race       US
    "MRCJ": { offset: 0x2673, from: 0x42, to: 0x48 }, // Race       JP
    "MSDE": { offset: 0x2A88, from: 0x42, to: 0x48 }, // Breeder    US
    "MSDJ": { offset: 0x2A88, from: 0x42, to: 0x48 }, // Breeder    JP
    "MSTJ": { offset: 0x4AE3, from: 0x42, to: 0x48 }, // Tetris     JP
    "MSTP": { offset: 0x4B18, from: 0x42, to: 0x48 }, // Tetris     EU
    "MTAE": { offset: 0x2A40, from: 0x42, to: 0x48 }, // Togepi     US
    "MTAJ": { offset: 0x2A40, from: 0x42, to: 0x48 }, // Togepi     JP
  //"MZ2J": { offset: 0x0000, from: 0x42, to: 0x48 }, // Puzzle 2   JP      //Not needed
  };
  const patch = PATCH_DATA[entry.gameCode];
  if (!patch) return entry.bytes; // no patch for this game
  const view = new Uint8Array(entry.bytes);
  if (view[patch.offset] === patch.from) {
    view[patch.offset] = patch.to;
    console.log(`[resume-patched] ${entry.gameCode} @ ${patch.offset.toString(16)}`);
  } else {
    console.warn(`[resume-skip] ${entry.gameCode} unexpected byte at ${patch.offset.toString(16)}`);
  }
  return entry.bytes;
}

// ===== Patch orchestration =====
async function runPatch() {
	if (!baseFirmware) {
		if (location.protocol === 'file:') {
			alert('Open from disk detected. Please choose a UF2 with “Pick another…” or click “Reload” to open the file picker.');
		} else {
			alert('Load base firmware first (PM2040.uf2 or pick manually).');
		}
		return;
	}
	if (!entries.length) {
		alert('Add at least one .min game.');
		return;
	}

	let anyEmpty = false;
	for (const tr of $$('#tbody tr')) {
		const inp = tr.querySelector('.name-input');
		const hint = tr.querySelector('.name-hint');
		showValidation(inp, hint);
		if (!inp.value) anyEmpty = true;
	}
	if (anyEmpty) {
		alert('Some names are empty.');
		return;
	}

	const ROMArray = entries.map(e => patchGameResumeBehavior(e));

	// Apply uppercase if toggle is on (patch-time), then enforce 14 chars
	const caps = isCapsOn();
	$$('#tbody tr').forEach(tr => {
		const inp = tr.querySelector('.name-input');
		if (caps) inp.value = inp.value.toUpperCase();
		inp.value = inp.value.slice(0, NAME_MAX);
	});

	// Ensure remaining slots are cleared in firmware
	window.ENTRIES = MAX_GAMES;

	try {
		if (typeof window.injectROMs === 'function') {
			await window.injectROMs(baseFirmware.slice(0), ROMArray);
		} else {
			alert('injectROMs(...) not found. Please include patcher.js before this UI script.');
		}
	} catch (err) {
		console.error('Patch error:', err);
		alert('Patch failed. See console for details.');
	}
}

// ===== Init =====
(function init() {
	initTheme();
	initCaps();
	initNameSource();
	tryLoadDefaultFirmware();

	$('#themeSel').addEventListener('change', (e) => applyTheme(e.target.value));
	$('#capsToggle').addEventListener('change', (e) => applyCaps(e.target.checked));

	const dz = $('#dropzone');
	const fileInput = $('#fileInput');

	['dragenter', 'dragover'].forEach(ev => dz.addEventListener(ev, e => {
		e.preventDefault();
		dz.classList.add('dragover');
	}));
	['dragleave', 'drop'].forEach(ev => dz.addEventListener(ev, e => {
		e.preventDefault();
		dz.classList.remove('dragover');
	}));
	dz.addEventListener('drop', e => {
		const files = e.dataTransfer?.files || [];
		addFiles(files);
	});
	fileInput.addEventListener('change', e => addFiles(e.target.files));

	$('#btnClear').addEventListener('click', () => {
		entries = [];
		$('#tbody').innerHTML = '';
		renumber();
	});

	$('#fwInput').addEventListener('change', async (e) => {
		const f = e.target.files?.[0];
		if (!f) return;
		if (!f.name.toLowerCase().endsWith('.uf2')) {
			alert('Only .uf2 is allowed.');
			e.target.value = '';
			setFwStatus('Base firmware: not found', 'err');
			return;
		}
		baseFirmware = await f.arrayBuffer();
		setFwStatus('Base firmware: loaded (manual)', 'ok');
		$('#fwInfo').textContent = 'Loaded "' + f.name + '" (' + formatSize(baseFirmware.byteLength) + ').';
	});

	$('#btnReloadFw').addEventListener('click', () => {
		if (location.protocol === 'file:') {
			$('#fwInput').click();
		} else {
			tryLoadDefaultFirmware();
		}
	});

	$('#btnRun').addEventListener('click', runPatch);

	// Global name source selector (persist + apply to rows)
	$('#globalNameSrc').addEventListener('change', (e) => {
		const val = e.target.value; // 'filename' | 'binary'
		applyNameSource(val); // persist selection
		$$('#tbody tr').forEach((tr) => {
			const id = parseInt(tr.dataset.id, 10);
			const entry = entries.find(en => en.id === id);
			if (!entry) return;
			const sel = tr.querySelector('.name-source');
			const nameInput = tr.querySelector('.name-input');
			const hint = tr.querySelector('.name-hint');
			if (val === 'binary' && !entry.binaryName) {
				sel.value = 'filename';
				entry.nameSource = 'filename';
				return;
			}
			sel.value = val;
			entry.nameSource = val;
			const newName = val === 'binary' ? entry.binaryName : defaultNameFromFilename(entry.filename);
			if (newName != null) {
				entry.name = newName;
				nameInput.value = newName;
				showValidation(nameInput, hint);
			}
		});
	});
})();
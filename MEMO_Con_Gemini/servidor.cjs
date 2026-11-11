'use strict';
const http = require('node:http');
const { timingSafeEqual } = require('node:crypto');

const apiKey = process.env.GEMINI_API_KEY;
const token = Buffer.from(process.env.MEMO_TOKEN || '');
const model = process.env.GEMINI_MODELO || 'gemini-3.8-flash';
const port = 8787;
const pcmBytes = 16000 * 3 * 2;
let busy = false;

if (!apiKey || token.length < 12 || !/^[a-zA-Z0-9.-]+$/.test(model)) {
  console.error('Configura GEMINI_API_KEY, un MEMO_TOKEN de al menos 12 caracteres y un modelo valido.');
  process.exit(1);
}

function reply(res, status, text) {
  res.writeHead(status, {
    'Content-Type': 'text/plain; charset=utf-8',
    'Content-Length': Buffer.byteLength(text),
    'Cache-Control': 'no-store'
  });
  res.end(text);
}

function wav(pcm) {
  const header = Buffer.alloc(44);
  header.write('RIFF', 0);
  header.writeUInt32LE(pcm.length + 36, 4);
  header.write('WAVEfmt ', 8);
  header.writeUInt32LE(16, 16);
  header.writeUInt16LE(1, 20);
  header.writeUInt16LE(1, 22);
  header.writeUInt32LE(16000, 24);
  header.writeUInt32LE(32000, 28);
  header.writeUInt16LE(2, 32);
  header.writeUInt16LE(16, 34);
  header.write('data', 36);
  header.writeUInt32LE(pcm.length, 40);
  return Buffer.concat([header, pcm]).toString('base64');
}

function displayText(text) {
  const normalized = text.normalize('NFD').replace(/\p{M}/gu, '').toUpperCase()
    .replace(/[^A-Z0-9 .,:?/-]/g, ' ').replace(/\s+/g, ' ').trim();
  if (normalized.length <= 410) return normalized;
  const cut = normalized.lastIndexOf(' ', 407);
  return normalized.slice(0, cut > 0 ? cut : 407) + '...';
}

const server = http.createServer(async (req, res) => {
  if (req.method === 'GET' && req.url === '/salud') {
    return reply(res, 200, 'MEMO SERVIDOR LISTO');
  }
  if (req.method !== 'POST' || req.url !== '/chat') return reply(res, 404, 'RUTA NO DISPONIBLE');

  const provided = Buffer.from(req.headers['x-memo-token'] || '');
  if (provided.length !== token.length || !timingSafeEqual(token, provided)) {
    return reply(res, 401, 'TOKEN INCORRECTO');
  }
  if (req.headers['content-type'] !== 'application/octet-stream' ||
      Number(req.headers['content-length']) !== pcmBytes) {
    return reply(res, 400, 'AUDIO INVALIDO');
  }
  if (busy) return reply(res, 409, 'MEMO OCUPADO');
  busy = true;
  try {
    const chunks = [];
    for await (const chunk of req) chunks.push(chunk);
    const pcm = Buffer.concat(chunks);
    if (pcm.length !== pcmBytes) return reply(res, 400, 'AUDIO INCOMPLETO');
    const upstream = await fetch(
      'https://generativelanguage.googleapis.com/v1beta/models/' + model + ':generateContent',
      {
        method: 'POST',
        headers: { 'Content-Type': 'application/json', 'x-goog-api-key': apiKey },
        signal: AbortSignal.timeout(22000),
        body: JSON.stringify({
          systemInstruction: { parts: [{ text: 'Eres MEMO, un asistente de estudio. Escucha la pregunta en el audio y responde en español, en texto plano, con un máximo de 45 palabras. Si no se entiende, pide que la repitan. No afirmes haber guardado tareas ni haber usado herramientas.' }] },
          contents: [{ parts: [{ inlineData: { mimeType: 'audio/wav', data: wav(pcm) } }] }],
          generationConfig: { maxOutputTokens: 512 }
        })
      }
    );
    if (!upstream.ok) {
      console.error('Gemini respondio HTTP', upstream.status);
      const status = upstream.status === 429 ? 429 :
        [400, 401, 403, 404].includes(upstream.status) ? 503 : 502;
      return reply(res, status, 'ERROR DE GEMINI');
    }
    const body = await upstream.json();
    const parts = body.candidates?.[0]?.content?.parts || [];
    const text = displayText(parts.filter(p => !p.thought && typeof p.text === 'string')
      .map(p => p.text).join(' '));
    reply(res, text ? 200 : 502, text || 'RESPUESTA VACIA');
  } catch (error) {
    console.error('Error de consulta:', error.name);
    if (!res.destroyed && !res.headersSent) reply(res, 502, 'ERROR DE CONEXION');
  } finally {
    busy = false;
  }
});
server.requestTimeout = 30000;
server.listen(port, '0.0.0.0', () => console.log('MEMO escucha en el puerto ' + port));

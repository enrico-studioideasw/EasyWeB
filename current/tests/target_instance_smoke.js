const fs = require('fs');
const vm = require('vm');

const html = fs.readFileSync(process.argv[2], 'utf8');
const scripts = [...html.matchAll(/<script>([\s\S]*?)<\/script>/g)].map(m => m[1]);
if (scripts.length !== 2) throw new Error('expected two target scripts');

function target() {
  return {
    attrs: {}, childNodes: [],
    hasAttribute(name) { return Object.prototype.hasOwnProperty.call(this.attrs, name); },
    setAttribute(name, value) { this.attrs[name] = value; },
    replaceChildren(...nodes) { this.childNodes = nodes; }
  };
}

const targets = [target(), target()];
const forms = {form1: {id: 'form1', action: ''}, form2: {id: 'form2', action: ''}};
const pending = {};
const context = {
  document: {
    getElementById(id) { return forms[id]; },
    querySelectorAll() { return targets; }
  },
  location: {href: '/'},
  FormData: function (form) { this.form = form; },
  fetch: function (_url, options) {
    return new Promise(resolve => { pending[options.body.form.id] = resolve; });
  },
  DOMParser: function () {
    this.parseFromString = text => ({
      getElementById: () => ({childNodes: [text]})
    });
  }
};

for (const script of scripts) vm.runInNewContext(script, context);
if (targets[0].attrs['data-ewb-target-instance'] !== 'form1' ||
    targets[1].attrs['data-ewb-target-instance'] !== 'form2')
  throw new Error('instances were not bound independently');

function response(text) {
  return {ok: true, text: async () => text};
}
function settle() {
  return new Promise(resolve => setImmediate(resolve));
}

(async function () {
  pending.form2(response('B'));
  await settle();
  if (targets[0].childNodes.length || targets[1].childNodes[0] !== 'B')
    throw new Error('response B changed the wrong instance');
  pending.form1(response('A'));
  await settle();
  if (targets[0].childNodes[0] !== 'A' || targets[1].childNodes[0] !== 'B')
    throw new Error('response A changed the wrong instance');
})().catch(error => { console.error(error); process.exitCode = 1; });

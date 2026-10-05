
# External editor communications

As of now, to execute code from an external editor, we can hack our way with
launching in REPL mode and sending code line by line or using the headless 
notebook-cell simulation to evaluate multiple lines with a `%%` termination 
line.

An editor integration of tzpl would involve spawning `tzpl_app` with 
`--ext-editor` + writing/reading it's stdio pipes.

jsonl protocol: Encode requests/results data in json objects following schema 
defined below. One object per line.

tzpl writes to stdout/stderr in a number of places:
  - println
  - compiler output

This means the editor need to handle non-json lines as well. 

Everything asynchronous / time sensitive is handled internally by tzpl. 
Editor just send eval objects and renders data without synchronisation needs.

## Plan

## Protocol

Each object must contain a "tag" key with a string value. The
object shape is determined by the tag table below.

Tag table:

handshake: 
  * editorName: string
  * documentPath: string

handshake_reply:
  * bare

eval:
  * source: string
  * documentPath: string

done:
  * bare - the ok reply to eval

result:
  * value: string,
  * typeName: string

error:
  * kind: string,
  * filename: string,
  * message: string,
  * position: { row: int, col: int }

protocol_error:
  * message: string

#### Conversation example

```json
ed -> { "tag": "handshake", "editorName": "vim", "documentPath": "~/test.x" }
tz -> { "tag": "handshake_reply" }

ed -> { "tag": "eval", "source": "let a = 0;", "documentPath": "~/test.x" }
tz -> { "tag": "done" } // eval ok

ed -> { "tag": "eval", "source": "a + 10;", "documentPath": "~/test.x" }
tz -> { "tag": "result", "value": "10", "typeName": "Int" }

ed -> { "tag": "eval", "source": "a + b;", "documentPath": "~/test.x" }
tz -> { "tag": "error", "kind": "type_error", "filename": "~/test.x", "message": "Undeclared identifier 'b'", "position": { "row": 1, "col": 5 } }

ed -> { "tag": "weird_tag", "source": 0, "documentPath": "~/test.x", "fun_key": "hahaha" }
tz -> { "tag": "protocol_error", "message": "Invalid tag 'weird_tag'" }
tz -> { "tag": "protocol_error", "message": "Invalid type for key 'source'. Expected string, got integer" }
tz -> { "tag": "protocol_error", "message": "Invalid key 'fun_key'" }
```

## Implementation

### Json parsing

Encode/decode data. Decoupled from protocol implementation.

**files:** 
  * `../app/src/jsonl.hpp`
  * `../app/src/jsonl.cpp`

**tests:**
  `../app/tests/tzpl_jsonl_test.cpp`

**parse primitives**:
- [x] string
- [x] number
- [x] boolean
- [x] array
- [x] object

**public api:**
  - [x] parse(request)
        Parse the json value
  - [ ] encode(reply) 
        Encode a reply as a json value

### Editor protocol

Implements protocol specifications.

files:
  * `../app/src/ext_editor.hpp`
  * `../app/src/ext_editor.cpp`

tests:
  `../app/tests/tzpl_ext_editor.cpp`

public api:
  * run()
  * readLine(stream)
  * sendReply(obj)

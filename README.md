# TPV — Terminal Typing Practice & Visualization

**TPV** is a fast, lightweight, and fun command-line typing game designed to help you improve your typing speed and accuracy.  
Practice typing words, sentences, or code snippets under customizable time limits — all from your terminal!

---

## Features
- **Typing Practice Game** — Type random words, sentences, or code snippets accurately and quickly.  
- **Timing & Statistics** — Measure your typing speed and accuracy in real time.  
- **Datasets Support** — Use built-in datasets or custom text files.  
- **Configurable Options** — Adjust difficulty, time limits, and game-over rules.  
- **Lightweight & Fast** — Runs entirely in the terminal, no dependencies required.

---

## Usage

```bash
tpv [options] [datasets...]
````

### Options

| Option                                           | Description                                         |
| ------------------------------------------------ | --------------------------------------------------- |
| `-h, --help`                                     | Show help message and exit.                         |
| `-i, --ignore-case`                              | Ignore letter casing when typing.                   |
| `-p, --ignore-punctuations`                      | Ignore punctuation differences.                     |
| `-r, --retry`                                    | Enable retry after mistakes.                        |
| `--[no-]game-over-on-mistake`                    | End game on the first mistake.                      |
| `--[no-]game-over-on-exceed-time-limit`          | End game when total time limit is exceeded.         |
| `--[no-]game-over-on-exceed-time-per-char-limit` | End game when per-character time limit is exceeded. |
| `--time-limit=<duration>`                        | Set total time limit (`1m`, `30s`, `2h10m`, etc.).  |
| `--time-per-char-limit=<duration>`               | Set per-character time limit (`500ms`, `2s`, etc.). |

---

## Datasets

You can specify one or more datasets as arguments.

* **Built-in datasets:** Use the `@` prefix (e.g. `@english-words`, `@code-snippets`).
* **Custom datasets:** Provide a path to a text file (each line = one prompt).

### Example:

```sh
tpv @english-words
tpv --time-limit=2m --retry my_dataset.txt
tpv -ip @code-snippets
tpv # using default @english-words dataset
```

To see available built-in datasets:

```bash
tpv @unknown
```

---

## Installation

### Quick install
```sh
# TODO
```

### Build from Source

```sh
git clone https://github.com/Maqi-x/tpv.git
cd tpv
make
```

After installation:

```sh
tpv --help
```

## License
This project is licensed under the **GNU GPL V3 License** — see the [LICENSE](LICENSE) file for details.

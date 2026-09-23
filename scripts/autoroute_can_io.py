"""Headless KiCad Specctra bridge used by the GitHub hardware workflow."""
from __future__ import annotations

import argparse
from pathlib import Path
import pcbnew


def clear_tracks(board: pcbnew.BOARD) -> None:
    for item in list(board.GetTracks()):
        board.Remove(item)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("export", "import"))
    parser.add_argument("board", type=Path)
    parser.add_argument("exchange", type=Path)
    args = parser.parse_args()
    board = pcbnew.LoadBoard(str(args.board))
    clear_tracks(board)
    if args.mode == "export":
        if not pcbnew.ExportSpecctraDSN(board, str(args.exchange)):
            raise SystemExit("Specctra DSN export failed")
    else:
        if not pcbnew.ImportSpecctraSES(board, str(args.exchange)):
            raise SystemExit("Specctra SES import failed")
        pcbnew.SaveBoard(str(args.board), board)


if __name__ == "__main__":
    main()

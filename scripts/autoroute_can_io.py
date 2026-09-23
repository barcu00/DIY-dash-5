"""Headless KiCad Specctra bridge used by the GitHub hardware workflow."""
from __future__ import annotations

import argparse
from pathlib import Path
import pcbnew


def clear_tracks(board: pcbnew.BOARD) -> None:
    for item in list(board.GetTracks()):
        board.Remove(item)


def remove_router_planes(dsn_path: Path, net_name: str) -> None:
    """Strip DSN plane expressions so the router completes the net as tracks."""
    text = dsn_path.read_text(encoding="utf-8")
    marker = f"(plane {net_name} "
    while marker in text:
        start = text.index(marker)
        depth = 0
        end = start
        for end in range(start, len(text)):
            if text[end] == "(":
                depth += 1
            elif text[end] == ")":
                depth -= 1
                if depth == 0:
                    end += 1
                    break
        else:
            raise ValueError(f"unterminated DSN plane for {net_name}")
        text = text[:start] + text[end:]
    dsn_path.write_text(text, encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("export", "import"))
    parser.add_argument("board", type=Path)
    parser.add_argument("exchange", type=Path)
    args = parser.parse_args()
    board = pcbnew.LoadBoard(str(args.board))
    clear_tracks(board)
    if args.mode == "export":
        clean_board = args.exchange.with_suffix(".unrouted.kicad_pcb")
        pcbnew.SaveBoard(str(clean_board), board)
        if not pcbnew.ExportSpecctraDSN(board, str(args.exchange)):
            raise SystemExit("Specctra DSN export failed")
        remove_router_planes(args.exchange, "POWER_GND")
    else:
        if not pcbnew.ImportSpecctraSES(board, str(args.exchange)):
            raise SystemExit("Specctra SES import failed")
        board.BuildConnectivity()
        if not pcbnew.ZONE_FILLER(board).Fill(board.Zones()):
            raise SystemExit("Copper zone fill failed")
        pcbnew.SaveBoard(str(args.board), board)


if __name__ == "__main__":
    main()

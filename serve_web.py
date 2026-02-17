#!/usr/bin/env python3
"""Serve the web build output for local testing.

Usage:
  python3 serve_web.py
  python3 serve_web.py --dir build-web --port 8080
"""

from __future__ import annotations

import argparse
import http.server
import os
import socketserver


def main() -> None:
    parser = argparse.ArgumentParser(description="Serve web build output")
    parser.add_argument("--dir", default="build-web", help="Directory to serve (default: build-web)")
    parser.add_argument("--port", type=int, default=8000, help="Port to listen on (default: 8000)")
    args = parser.parse_args()

    web_dir = os.path.abspath(args.dir)
    if not os.path.isdir(web_dir):
        raise SystemExit(f"Directory not found: {web_dir}")

    os.chdir(web_dir)

    handler = http.server.SimpleHTTPRequestHandler
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", args.port), handler) as httpd:
        print(f"Serving {web_dir}")
        print(f"Open http://localhost:{args.port}/animation.html")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nStopped")


if __name__ == "__main__":
    main()

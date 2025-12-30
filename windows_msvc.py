#!/usr/bin/env python
"""
Patched Windows platform tool that prioritizes MSVC detection.
Copy this to the project root as a fallback if the automatic detection fails.
"""

import os
import sys

# Add godot-cpp tools to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "godot-cpp", "tools"))

from godot_cpp.tools import windows as original_windows

def generate_with_msvc_fix(env):
    """Wrapper that ensures MSVC is detected and used."""
    
    # Force environment detection
    if sys.platform == "win32":
        # Try to set up Visual Studio environment variables
        vswhere = os.path.join(
            os.environ.get("ProgramFiles(x86)", "C:\\Program Files (x86)"),
            "Microsoft Visual Studio", "Installer", "vswhere.exe"
        )
        
        if os.path.exists(vswhere):
            import subprocess
            try:
                vs_path = subprocess.check_output(
                    [vswhere, "-latest", "-products", "*", 
                     "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                     "-property", "installationPath"],
                    text=True
                ).strip()
                
                if vs_path:
                    vcvarsall = os.path.join(vs_path, "VC", "Auxiliary", "Build", "vcvarsall.bat")
                    if os.path.exists(vcvarsall):
                        # Setup environment
                        env_setup = subprocess.check_output(
                            f'"{vcvarsall}" x64 && set',
                            shell=True,
                            text=True
                        )
                        
                        for line in env_setup.split('\n'):
                            if '=' in line:
                                key, val = line.split('=', 1)
                                os.environ[key] = val
                                env[key] = val
            except Exception as e:
                print(f"Warning: Could not auto-setup MSVC: {e}")
    
    # Ensure use_mingw is False to force MSVC
    env["use_mingw"] = False
    
    # Call original windows.py generate, which should now find MSVC
    original_windows.generate(env)

# For direct SConstruct usage
def options(opts):
    original_windows.options(opts)

def exists(env):
    return original_windows.exists(env)

def generate(env):
    generate_with_msvc_fix(env)

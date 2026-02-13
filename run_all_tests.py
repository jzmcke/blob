import subprocess
import os
import sys
import platform

def run_command(command, cwd, description):
    print(f"--- {description} ---")
    print(f"CWD: {cwd}")
    print(f"CMD: {command}")
    try:
        # shell=True for windows to handle commands like 'npm' or 'cmake' better if not in direct path sometimes, 
        # but list format is usually safer. However, for 'npm', 'cmake', shell=True is often easiest on Windows.
        if platform.system() == "Windows":
             result = subprocess.run(command, cwd=cwd, shell=True, check=False)
        else:
             result = subprocess.run(command.split(), cwd=cwd, check=False)
             
        if result.returncode == 0:
            print(f"--- PASSED: {description} ---\n")
            return True
        else:
            print(f"--- FAILED: {description} (Exit Code: {result.returncode}) ---\n")
            return False
    except Exception as e:
        print(f"--- ERROR: {description} ({e}) ---\n")
        return False

def main():
    root_dir = os.path.dirname(os.path.abspath(__file__)) # c:\git\website\blob
    website_root = os.path.dirname(root_dir) # c:\git\website
    
    steps = [
        {
            "desc": "Build C Project (CMake)",
            "cmd": "cmake --build . --config Release",
            "cwd": os.path.join(root_dir, "build")
        },
        {
            "desc": "Run C Unit Tests",
            "cmd": r"..\bin\Release\blob_unit_test.exe",
            "cwd": os.path.join(root_dir, "build")
        },
        {
            "desc": "Run C Network Tests (Loopback)",
            "cmd": r"..\bin\Release\blob_network_test.exe",
            "cwd": os.path.join(root_dir, "build")
        },
        {
            "desc": "Run Loopback Integration Test (Requires core-server)",
            "cmd": r"..\bin\Release\blob_loopback_test.exe",
            "cwd": os.path.join(root_dir, "build")
        },
        {
            "desc": "Run C Triangle wave test (Smoke)",
            "cmd": r"..\bin\Release\blob_triangle_test.exe 1000",
            "cwd": os.path.join(root_dir, "build")
        },
        {
            "desc": "Run Python CFFI Tests",
            "cmd": "python test_blob_core.py",
            "cwd": os.path.join(root_dir, "test")
        },
        {
            "desc": "Run JavaScript Blob Decoder Tests",
            "cmd": "npx jest blob.test.js",
            "cwd": os.path.join(root_dir, "test", "js")
        }
    ]

    failed_steps = []
    
    for step in steps:
        # Special handling for JS tests - auto-install dependencies if needed
        if step["desc"] == "Run JavaScript Blob Decoder Tests":
            js_test_dir = step["cwd"]
            node_modules_path = os.path.join(js_test_dir, "node_modules")
            
            # Check if node_modules exists, if not, run npm install
            if not os.path.exists(node_modules_path):
                print(f"--- Installing JavaScript test dependencies ---")
                print(f"CWD: {js_test_dir}")
                print(f"CMD: npm install")
                try:
                    install_result = subprocess.run("npm install", cwd=js_test_dir, shell=True, check=False)
                    if install_result.returncode != 0:
                        print(f"--- SKIPPED: {step['desc']} (npm install failed) ---\n")
                        continue
                    print(f"--- Dependencies installed successfully ---\n")
                except Exception as e:
                    print(f"--- SKIPPED: {step['desc']} (npm install error: {e}) ---\n")
                    continue
        
        if not run_command(step["cmd"], step["cwd"], step["desc"]):
            failed_steps.append(step["desc"])

    print("========================================")
    print("TEST SUMMARY")
    print("========================================")
    if not failed_steps:
        print("ALL TESTS PASSED")
        sys.exit(0)
    else:
        print("THE FOLLOWING TESTS FAILED:")
        for s in failed_steps:
            print(f"- {s}")
        sys.exit(1)

if __name__ == "__main__":
    main()

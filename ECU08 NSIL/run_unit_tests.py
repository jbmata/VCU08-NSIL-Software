#!/usr/bin/env python3
"""
Compilador y ejecutor de tests unitarios - ECU08 NSIL
"""
import subprocess
import os
import sys
import json
from pathlib import Path

class TestRunner:
    def __init__(self):
        self.project_root = Path(__file__).parent.parent.parent
        self.build_dir = self.project_root / "build_tests"
        self.test_executable = self.build_dir / "ecu08_unit_tests.exe"
        
    def setup_build(self):
        """Crear directorio de build"""
        self.build_dir.mkdir(exist_ok=True)
        return True
        
    def compile_tests(self):
        """Compilar tests con CMake"""
        print("[*] Checking for CMake...")
        try:
            result = subprocess.run(
                ["cmake", "--version"],
                capture_output=True,
                timeout=5
            )
            if result.returncode != 0:
                print("[-] CMake not found in PATH")
                return False
        except:
            print("[-] CMake not available")
            return False
            
        print("[+] CMake found")
        print("[*] Configuring CMake...")
        
        cmake_config = subprocess.run(
            ["cmake", "-B", str(self.build_dir), "-DBUILD_UNIT_TESTS=ON", "-DBUILD_SIL_TESTS=OFF"],
            cwd=str(self.project_root),
            capture_output=True,
            text=True
        )
        
        if cmake_config.returncode != 0:
            print("[-] CMake configuration failed:")
            print(cmake_config.stderr)
            return False
            
        print("[+] CMake configured successfully")
        print("[*] Building tests...")
        
        cmake_build = subprocess.run(
            ["cmake", "--build", str(self.build_dir), "--config", "Debug"],
            cwd=str(self.project_root),
            capture_output=True,
            text=True
        )
        
        if cmake_build.returncode != 0:
            print("[-] Build failed:")
            print(cmake_build.stderr)
            return False
            
        print("[+] Build completed successfully")
        return True
        
    def run_tests(self):
        """Ejecutar tests unitarios"""
        if not self.test_executable.exists():
            print("[-] Test executable not found:", self.test_executable)
            return False
            
        print("[*] Running unit tests...")
        print("-" * 60)
        
        result = subprocess.run(
            str(self.test_executable),
            capture_output=False,
            text=True
        )
        
        print("-" * 60)
        
        if result.returncode == 0:
            print("[+] All tests PASSED")
            return True
        else:
            print("[-] Some tests FAILED")
            return False
            
    def main(self):
        print("=" * 60)
        print("ECU08 NSIL - Unit Tests Runner")
        print("=" * 60)
        print()
        
        if not self.setup_build():
            return 1
            
        if not self.compile_tests():
            print("\n[!] Compilation failed. Check logs above.")
            return 1
            
        if not self.run_tests():
            print("\n[!] Tests failed. Check output above.")
            return 1
            
        print()
        print("=" * 60)
        print("[SUCCESS] All tests passed!")
        print("=" * 60)
        return 0

if __name__ == "__main__":
    runner = TestRunner()
    sys.exit(runner.main())

from conan import ConanFile
from conan.tools.cmake import cmake_layout


class AsyncPsqlRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("libpqxx/8.0.2")

    def layout(self):
        cmake_layout(self)
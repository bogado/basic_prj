// fileysystem.hpp                                                                        -*-C++-*-
#ifndef INCLUDED_FILEYSYSTEM_HPP
#define INCLUDED_FILEYSYSTEM_HPP

#include <filesystem>

#include <fmt/format.h>
#include <fmt/std.h>

#include <sys/errno.h>

#include "./string.hpp"

namespace vb {

// NOLINTNEXTLINE(unsued_namespace)
namespace fs = std::filesystem;
    
struct base_filesystem_ops {
    virtual ~base_filesystem_ops() = default;
    virtual bool create_directories(fs::path) const = 0;
    virtual std::error_code create_link(fs::path, fs::path) const = 0;
    virtual std::error_code create_symlink(fs::path, fs::path) const = 0;
    virtual std::error_code copy(fs::path, fs::path) const = 0;
    virtual std::error_code move(fs::path, fs::path) const = 0;
    virtual std::error_code unlink(fs::path) const = 0;
};

template <typename PATH_LIKE>
concept is_path_like = std::same_as<PATH_LIKE, fs::path> || is_string_type<PATH_LIKE>; 

struct filesystem_mock_ops : base_filesystem_ops {
    bool create_directories(fs::path path) const override {
        fmt::print("Create directories {}\n", path);
        return true;
    }

    std::error_code create_link(fs::path source, fs::path target) const override
    {
        fmt::print("Create link {} -> {}\n", source, target);
        return std::error_code{};
    }

    std::error_code create_symlink(fs::path source, fs::path target) const override {
        fmt::print("Create symlink {} -> {}\n", source, target);
        return std::error_code{};
    }

    std::error_code copy(fs::path source, fs::path target) const override {
        fmt::print("Copy {} -> {}\n", source, target);
        return std::error_code{};
    }

    std::error_code move(fs::path source, fs::path target) const override {
        fmt::print("Move {} -> {}\n", source, target);
        return std::error_code{};
    }

    std::error_code unlink(fs::path file) const override {
        fmt::print("Remove {}\n", file);
        return std::error_code{};
    }
};

struct filesystem_ops : base_filesystem_ops {
    bool create_directories(fs::path target) const override {
        return fs::create_directories(target);
    }

    std::error_code create_symlink(fs::path source, fs::path target) const override {
        std::error_code error;
        fs::create_symlink(source, target, error);
        return error;
    }

    std::error_code create_link(fs::path source, fs::path target) const override {
        std::error_code error;
        fs::create_hard_link(source, target, error);
        return error;
    }

    std::error_code copy(fs::path source, fs::path target) const override {
        std::error_code error;
        fs::copy_file(source, target, error);
        return error;
    }

    std::error_code move(fs::path source, fs::path target) const override {
        std::error_code error;
        fs::rename(source, target, error);
        return error;
    }

    std::error_code unlink(fs::path file) const override {
        std::error_code error;
        fs::remove(file, error);
        return error;
    }
};

struct filesystem {
    enum mode {
        MOCK,
        EXEC};

    static inline auto pimpl = std::variant<filesystem_mock_ops, filesystem_ops>{};

    static void setMode(mode new_mode) {
        switch(new_mode) {
        case MOCK:
            pimpl = filesystem_mock_ops{};
            break;
        case EXEC:
            pimpl = filesystem_ops{};
            break;
        }
    }

    static bool create_directories(fs::path source)
    {
        return std::visit([&](auto& impl) {
            return impl.create_directories(source);
        }, pimpl);
    }

    static std::error_code create_link(fs::path source, fs::path target)
    {
        return std::visit([&](auto& impl) {
            return impl.create_link(source, target);
        }, pimpl);
    }

    static std::error_code create_symlink(fs::path source, fs::path target)
    {
        return std::visit([&](auto& impl) {
            return impl.create_symlink(source, target);
        }, pimpl);
    }

    static std::error_code copy(fs::path source, fs::path target)
    {
        return std::visit([&](auto& impl) {
            return impl.copy(source, target);
        }, pimpl);
    }

    static std::error_code move(fs::path source, fs::path target)
    {
        return std::visit([&](auto& impl) {
            return impl.move(source, target);
        }, pimpl);
    }

    static std::error_code unlink(fs::path file)
    {
        return std::visit([&](auto& impl) {
            return impl.unlink(file);
        }, pimpl);
    }
};

}

#endif

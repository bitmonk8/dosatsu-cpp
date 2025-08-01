add_rules("mode.debug", "mode.release")
set_defaultmode("debug")

add_moduledirs("xmake_modules")

add_repositories("myrepo https://github.com/bitmonk8/xmake-repo.git")
add_requires("libllvm 19.1.7", {
    system = false,   -- don’t search the host
    external = false, -- don’t download pre-built archives
    build = true,     -- build from source
    debug = is_mode("debug")
    })

if is_mode("debug") then
    set_runtimes("MDd")
    set_symbols("debug")
    set_optimize("none")
elseif is_mode("release") then
    set_runtimes("MT")
    set_symbols("hidden")
    set_optimize("fastest")
    set_strip("all")
end

set_languages("cxx20")
set_warnings("error")
function apply_common_flags()
    if is_plat("windows") then
        add_cxxflags("/wd4146")
    end
end

target("MakeIndex")
    set_kind("binary")
    add_files("MakeIndex/MakeIndex.cpp")
    add_files("MakeIndex/KuzuDump.cpp")
    add_packages("libllvm")
    add_includedirs("3rdParty/include", {public = true})
    apply_common_flags()
    
    -- Add test support - runs MakeIndex with --selftest flag
    add_tests("selftest", {
        run_timeout = 10000,
        runargs = {"--selftest"}
    })

-- Format target for code formatting with clang-format
target("format")
    set_kind("phony")
    on_run(function (target)
        -- Find all .h and .cpp files in the specified directories
        local source_dirs = {"MakeIndex"}
        local file_patterns = {"*.h", "*.cpp"}
        local files = {}
        
        for _, dir in ipairs(source_dirs) do
            if os.isdir(dir) then
                for _, pattern in ipairs(file_patterns) do
                    local found_files = os.files(path.join(dir, pattern))
                    for _, file in ipairs(found_files) do
                        table.insert(files, file)
                    end
                end
            end
        end
        
        if #files == 0 then
            print("No source files found to format")
            return
        end
        
        print("Formatting " .. #files .. " source files...")
        
        -- Run clang-format on all found files
        local clang_format_cmd = "clang-format -i"
        for _, file in ipairs(files) do
            clang_format_cmd = clang_format_cmd .. " " .. file
        end
        
        -- Execute the formatting command
        local ok, errors = os.iorunv("clang-format", table.join({"-i"}, files))
        if not ok then
            print("Error running clang-format: " .. (errors or "unknown error"))
            print("Make sure clang-format is installed and available in PATH")
            os.exit(1)
        end
        
        print("Code formatting completed successfully!")
        print("Formatted files in directories: " .. table.concat(source_dirs, ", "))
    end)

-- Lint target for code linting with clang-tidy
target("lint")
    set_kind("phony")
    on_run(function (target)
        import("core.base.option")
        import("execv")
        local args = option.get("arguments")
        local file_to_lint = args and args[1]

        local files = {}
        if file_to_lint and file_to_lint ~= "" then
            if os.isfile(file_to_lint) then
                table.insert(files, file_to_lint)
                print("Running clang-tidy on " .. file_to_lint .. "...")
            else
                print("Error: File not found at: " .. file_to_lint)
                os.exit(1)
            end
        else
            print("Running clang-tidy on all project files...")
            local source_dirs ={"MakeIndex"}
            local file_patterns = {"*.cpp"}
            for _, dir in ipairs(source_dirs) do
                if os.isdir(dir) then
                    for _, pattern in ipairs(file_patterns) do
                        local found_files = os.files(path.join(dir, pattern))
                        for _, file in ipairs(found_files) do
                            table.insert(files, file)
                        end
                    end
                end
            end
        end

        if #files == 0 then
            print("No source files found to lint")
            return
        end

        if not os.isdir("build") then
            os.mkdir("build")
        end

        os.run("xmake project -k compile_commands build")

        local db_path = "build/compile_commands.json"
        local f_read = io.open(db_path, "r")
        if f_read then
            local content = f_read:read("*a")
            f_read:close()

            content = content:gsub("%%s?-Wno%%-unused%%-command%%-line%%-argument", "")
            content = content:gsub("%%s?-Werror", "")
            content = content:gsub('"([^"]*)"', function(match)
                return '"' .. match .. ' -Wno-deprecated-this-capture -Wno-deprecated-anon-enum-enum-conversion"'
            end)
            local f_write = io.open(db_path, "w")
            if f_write then
                f_write:write(content)
                f_write:close()
                print("Temporarily sanitized compile_commands.json for linting.")
            end
        end

        local fix_command_args = {"-p", "build", "--fix", "--fix-errors", "--quiet"}
        for _, file in ipairs(files) do
            table.insert(fix_command_args, file)
        end

        local nullLogFile = os.tmpfile()
        execv.MyExecV("clang-tidy", fix_command_args, {stdout = nullLogFile, stderr = nullLogFile})
        os.rm(nullLogFile)

        local command_args = {"-p", "build"}
        for _, file in ipairs(files) do
            table.insert(command_args, file)
        end

        local args_string = os.args(command_args)
        local cmdline = "clang-tidy " .. args_string
        print(cmdline)

        local out_file    = path.join("build", "lint_output.txt")
        local ok, errors = execv.MyExecV("clang-tidy", command_args, {stdout = out_file, stderr = out_file})

        local function filter_clang_tidy_output(output)
            local filtered = {}
            for line in output:gmatch("[^\r\n]+") do
                if not line:match("Suppressed %d+ warnings %(%d+ in non%-user code, %d+ NOLINT%)") and
                   not line:match("Use %-header%-filter=%.%* to display errors from all non%-system headers. Use %-system%-headers to display errors from system headers as well.") then
                    table.insert(filtered, line)
                end
            end
            return table.concat(filtered, "\n")
        end

        local lint_output = io.readfile(out_file)
        if lint_output and #lint_output > 0 then
            print(filter_clang_tidy_output(lint_output))
        end

        os.run("xmake project -k compile_commands build")

        if ok == 0 then
            print("\nCode linting completed successfully!")
        else
            print("\nCode linting completed with errors.")
        end
    end)

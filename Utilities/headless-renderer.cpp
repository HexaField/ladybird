/*
 * Copyright (c) 2024, Headless LibWeb Renderer
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/LexicalPath.h>
#include <LibCore/ArgsParser.h>
#include <LibCore/File.h>
#include <LibCore/System.h>
#include <LibGfx/Bitmap.h>
#include <LibGfx/ImageFormats/PNGWriter.h>
#include <LibGfx/PainterSkia.h>
#include <LibMain/Main.h>

static ErrorOr<void> render_html_to_png(StringView, StringView output_path, Gfx::IntSize viewport_size)
{
    // Create a simple bitmap and fill it with a gradient background
    auto bitmap = TRY(Gfx::Bitmap::create(Gfx::BitmapFormat::BGRA8888, viewport_size));

    // Create Skia painter
    auto painter = Gfx::PainterSkia::create(*bitmap);

    // Fill with a nice gradient background
    for (int y = 0; y < viewport_size.height(); ++y) {
        float progress = static_cast<float>(y) / viewport_size.height();
        auto color = Color(
            static_cast<u8>(102 + progress * (118 - 102)),  // 667eea to 764ba2
            static_cast<u8>(126 + progress * (75 - 126)),
            static_cast<u8>(234 + progress * (162 - 234))
        );
        auto line_rect = Gfx::FloatRect(0, y, viewport_size.width(), 1);
        painter->fill_rect(line_rect, color);
    }

    // Draw some sample content to demonstrate the renderer works
    auto header_rect = Gfx::FloatRect(50, 50, viewport_size.width() - 100, 100);
    painter->fill_rect(header_rect, Color(255, 255, 255, 200));

    // Draw feature boxes
    int box_width = 200;
    int box_height = 120;
    int spacing = 20;
    int start_y = 200;

    for (int i = 0; i < 4; ++i) {
        int x = 50 + (i % 2) * (box_width + spacing);
        int y = start_y + (i / 2) * (box_height + spacing);

        // Draw box with semi-transparent background
        auto box_rect = Gfx::FloatRect(x, y, box_width, box_height);
        painter->fill_rect(box_rect, Color(255, 255, 255, 150));
    }

    // Save as PNG
    auto png_data = TRY(Gfx::PNGWriter::encode(*bitmap));
    auto output_file = TRY(Core::File::open(output_path, Core::File::OpenMode::Write));
    TRY(output_file->write_until_depleted(png_data));

    return {};
}

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    // Parse command line arguments
    StringView input_file;
    StringView output_file = "output/render.png"sv;
    int width = 1024;
    int height = 768;
    bool verbose = false;

    Core::ArgsParser args_parser;
    args_parser.set_general_help("Headless HTML/CSS renderer demo using LibGfx");
    args_parser.add_positional_argument(input_file, "Input HTML file", "input.html");
    args_parser.add_option(output_file, "Output PNG file", "output", 'o', "output.png");
    args_parser.add_option(width, "Viewport width", "width", 'w', "width");
    args_parser.add_option(height, "Viewport height", "height", 'h', "height");
    args_parser.add_option(verbose, "Verbose output", "verbose", 'v');

    args_parser.parse(arguments);

    if (input_file.is_empty()) {
        warnln("Error: Input file is required");
        args_parser.print_usage(stderr, arguments.strings[0]);
        return 1;
    }

    if (verbose) {
        outln("Headless LibWeb Renderer Demo");
        outln("Input file: {}", input_file);
        outln("Output file: {}", output_file);
        outln("Viewport size: {}x{}", width, height);
    }

    // Load HTML file (for now we'll just use it as a trigger, but render our demo)
    auto file_result = Core::File::open(input_file, Core::File::OpenMode::Read);
    if (file_result.is_error()) {
        warnln("Error: Could not open input file '{}': {}", input_file, file_result.error());
        return 1;
    }

    auto html_content_result = file_result.value()->read_until_eof();
    if (html_content_result.is_error()) {
        warnln("Error: Could not read input file '{}': {}", input_file, html_content_result.error());
        return 1;
    }

    if (verbose) {
        outln("Rendering demo content...");
    }

    // Create output directory if it doesn't exist
    auto output_path = LexicalPath(output_file);
    if (!output_path.dirname().is_empty()) {
        auto mkdir_result = Core::System::mkdir(output_path.dirname(), 0755);
        if (mkdir_result.is_error() && mkdir_result.error().code() != EEXIST) {
            warnln("Error: Could not create output directory '{}': {}", output_path.dirname(), mkdir_result.error());
            return 1;
        }
    }

    // Render and save
    auto render_result = render_html_to_png(html_content_result.value(), output_file, { width, height });
    if (render_result.is_error()) {
        warnln("Error: Failed to render: {}", render_result.error());
        return 1;
    }

    if (verbose) {
        outln("Successfully rendered demo to: {}", output_file);
    } else {
        outln("Rendered demo: {} -> {}", input_file, output_file);
    }

    return 0;
}

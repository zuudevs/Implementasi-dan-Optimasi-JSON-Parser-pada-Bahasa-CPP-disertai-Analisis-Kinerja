/**
 * @file benchmark.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief High-Precision Benchmarks JSON Parser
 * @version 0.1.0
 * @date 2026-06-05
 *
 * @copyright Copyright (c) 2026
 */

#include "cpp_json/json.hpp"
#include "parser/parser.hpp"
#include "tokenizer/tokenizer.hpp"
#include <benchmark/benchmark.h>
#include <string>

static constexpr std::string_view kJsonData =
    R"({
    "web-app": {
        "servlet": [
            {
                "servlet-name": "cofaxCDS",
                "servlet-class": "org.cofax.cds.CDSServlet",
                "init-param": {
                    "configGlossary:installationAt": "Philadelphia, PA",
                    "configGlossary:adminEmail": "ksm@pobox.com",
                    "configGlossary:poweredBy": "Cofax",
                    "configGlossary:poweredByIcon": "/images/cofax.gif",
                    "configGlossary:staticPath": "/content/static",
                    "templateProcessorClass": "org.cofax.WysiwygTemplate",
                    "templateLoaderClass": "org.cofax.FilesTemplateLoader",
                    "templatePath": "templates"
                }
            },
            {
                "servlet-name": "cofaxAdmin",
                "servlet-class": "org.cofax.cds.AdminServlet"
            },
            {
                "servlet-name": "cofaxTools",
                "servlet-class": "org.cofax.cms.CofaxToolsServlet",
                "init-param": {
                    "templatePath": "toolstemplates/",
                    "log": 1,
                    "logLocation": "/usr/local/tomcat/logs/CofaxTools.log",
                    "logMaxSize": "",
                    "dataLog": 1,
                    "dataLogLocation": "/usr/local/tomcat/logs/dataLog.log",
                    "dataLogMaxSize": "",
                    "removePageCache": "/content/admin/remove?cache=pages&id=",
                    "removeTemplateCache": "/content/admin/remove?cache=templates&id=",
                    "fileTransferFolder": "/usr/local/tomcat/webapps/content/fileTransferFolder",
                    "lookInContext": 1,
                    "adminGroupID": 4,
                    "betaServer": true
                }
            }
        ],
        "servlet-mapping": {
            "cofaxCDS": "/",
            "cofaxEmail": "/cofaxutil/aemail/*",
            "cofaxAdmin": "/admin/*",
            "fileServlet": "/static/*",
            "cofaxTools": "/tools/*"
        },
        "taglib": {
            "taglib-uri": "cofax.tld",
            "taglib-location": "/WEB-INF/tlds/cofax.tld"
        }
    }
})";

// ── BENCHMARK 1: Tokenizer murni ──
static void BM_Tokenizer(benchmark::State& state) {
    const auto raw = std::span<const char>(kJsonData.data(), kJsonData.size());
    const size_t bytes_processed = raw.size();

    while (state.KeepRunning()) {
        auto tokens = zuu::tokenizer::Tokenizer::Tokenize(raw);
        if (!tokens) {
            std::string err_msg = "Tokenizer failed during execution! Error Code: " +
                                  std::to_string(static_cast<int>(tokens.error()));
            state.SkipWithError(err_msg.c_str());
            break;
        }
        benchmark::DoNotOptimize(tokens);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * bytes_processed);
}

// ── BENCHMARK 2: Parser murni ──
static void BM_ParserOnly(benchmark::State& state) {
    const auto raw = std::span<const char>(kJsonData.data(), kJsonData.size());

    auto tokens_result = zuu::tokenizer::Tokenizer::Tokenize(raw);
    if (!tokens_result) {
        std::string err_msg = "Tokenizer failed during setup! Error Code: " +
                              std::to_string(static_cast<int>(tokens_result.error()));
        state.SkipWithError(err_msg.c_str());
        return;
    }

    const size_t nodes_count = tokens_result->first.size();

    for (auto _ : state) {
        zuu::parser::Parser parser(tokens_result.value());
        auto parsed = std::move(parser).result();

        if (!parsed) {
            std::string err_msg = "Parser failed during DOM Construction! Error Code: " +
                                  std::to_string(static_cast<int>(parsed.error()));
            state.SkipWithError(err_msg.c_str());
            break;
        }
        benchmark::DoNotOptimize(parsed);
    }

    state.counters["Tokens/s"] = benchmark::Counter(static_cast<double>(nodes_count),
                                                    benchmark::Counter::kIsIterationInvariantRate);
}

// ── BENCHMARK 3: Pipeline Penuh (End-to-End) ──
static void BM_FullPipeline(benchmark::State& state) {
    const size_t bytes_processed = kJsonData.size();

    for (auto _ : state) {
        auto json = zuu::Json::parse(kJsonData);
        if (!json) {
            std::string err_msg = "Full Pipeline failed! Error Code: " +
                                  std::to_string(static_cast<int>(json.error()));
            state.SkipWithError(err_msg.c_str());
            break;
        }
        benchmark::DoNotOptimize(json);
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * bytes_processed);
}

// Registrasi Benchmark: Minimal 2 detik dan format ke Mikrodetik
BENCHMARK(BM_Tokenizer)->Unit(benchmark::kMicrosecond)->MinTime(2.0);
BENCHMARK(BM_ParserOnly)->Unit(benchmark::kMicrosecond)->MinTime(2.0);
BENCHMARK(BM_FullPipeline)->Unit(benchmark::kMicrosecond)->MinTime(2.0);

BENCHMARK_MAIN();
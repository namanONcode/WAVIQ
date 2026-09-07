// Minimal CLI harness for Module 1. This exercises the loaders end to end
// without a GUI yet: it stands in for the "GUI asks the user for manual
// metadata" flow using command-line flags instead of a dialog.
//
// Usage:
//   module1_cli <file.wav>
//   module1_cli <file.iq>                                 (uses sidecar if present)
//   module1_cli <file.iq> --sample-rate 2000000 --datatype int16 --byte-order little

#include <complex>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>

#include "module1/IqLoader.h"
#include "module1/LoaderExceptions.h"
#include "module1/LoaderFactory.h"
#include "module1/SignalTypes.h"

using namespace module1;

namespace {

std::optional<std::string> get_flag(int argc, char** argv, const std::string& name) {
    for (int i = 2; i < argc - 1; ++i) {
        if (name == argv[i]) return std::string(argv[i + 1]);
    }
    return std::nullopt;
}

void print_metadata(const SignalMetadata& m) {
    std::cout << "  source_format   : " << m.source_format << "\n";
    std::cout << "  sample_rate_hz  : " << m.sample_rate_hz << "\n";
    std::cout << "  sample_datatype : " << m.sample_datatype << "\n";
    std::cout << "  channel_count   : " << m.channel_count << "\n";
    std::cout << "  is_complex      : " << (m.is_complex ? "true" : "false") << "\n";
    std::cout << "  byte_order      : " << (m.byte_order == ByteOrder::Little ? "little" : "big") << "\n";
    std::cout << "  metadata_source : " << m.metadata_source << "\n";
}

void print_signal(const ComplexSignal& sig) {
    std::cout << "Loaded OK.\n";
    print_metadata(sig.metadata());
    std::cout << "  sample_count    : " << sig.sample_count() << "\n";

    const size_t preview = std::min<size_t>(5, sig.sample_count());
    std::cout << "  first " << preview << " samples: ";
    for (size_t i = 0; i < preview; ++i) {
        const auto& s = sig.samples()[i];
        std::cout << "(" << s.real() << (s.imag() >= 0 ? "+" : "") << s.imag() << "j) ";
    }
    std::cout << "\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file.wav|file.iq> "
                  << "[--sample-rate HZ --datatype int8|int16|float32 --byte-order little|big]\n";
        return 1;
    }

    const std::string path = argv[1];

    try {
        auto loader = make_loader_for(path);

        try {
            print_signal(loader->load(path));
            return 0;
        } catch (const MissingMetadataError& e) {
            // This is the branch a GUI would replace with a metadata dialog.
            auto sample_rate = get_flag(argc, argv, "--sample-rate");
            auto datatype = get_flag(argc, argv, "--datatype");
            auto byte_order = get_flag(argc, argv, "--byte-order");

            if (!sample_rate || !datatype) {
                std::cerr << "Missing metadata for " << path << ": " << e.what() << "\n";
                std::cerr << "Missing fields: ";
                for (const auto& f : e.missing_fields()) std::cerr << f << " ";
                std::cerr << "\nSupply them with --sample-rate --datatype [--byte-order].\n";
                return 1;
            }

            IqMetadataInput meta;
            meta.sample_rate_hz = std::stod(*sample_rate);
            meta.sample_datatype = *datatype;
            meta.byte_order = byte_order.value_or("little");
            meta.source = "manual_entry";

            auto* iq_loader = dynamic_cast<IqLoader*>(loader.get());
            if (!iq_loader) {
                std::cerr << "Manual metadata was supplied for a non-IQ file.\n";
                return 1;
            }

            print_signal(iq_loader->load_with_metadata(path, meta));
            return 0;
        }
    } catch (const SignalLoaderError& e) {
        std::cerr << "Error loading " << path << ": " << e.what() << "\n";
        return 1;
    }
}

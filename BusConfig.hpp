namespace PjonHL
{

/// Struct representing a PJON bus configuration. All participants in a network
/// should have the same configuration.
/// You can optionally pass an instance of the BusConfig into the PjonHL constructor.
/// All config options will be forwarded to the PJON backend.
/// For detailed documentation please have a look at the PJON protocol spec.
struct BusConfig
{
    enum class BusTopology
    {
        Local,
        Shared
    };

    enum class AckType
    {
        AckEnabled,
        AckDisabled
    };

    enum class CommunicationMode
    {
        Simplex,
        HalfDuplex
    };

    enum class CrcType
    {
        Crc8,
        Crc32
    };

    BusTopology       busTopology       = BusTopology::Local;
    CommunicationMode communicationMode = CommunicationMode::HalfDuplex;
    AckType           ackType           = AckType::AckEnabled;
    CrcType           crcType           = CrcType::Crc8;

    // Mac not yet suppored
};

}

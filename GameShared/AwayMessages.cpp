#include "pch.h"

#include "AwayMessages.h"
#include "BitReader.h"
#include "BitWriter.h"

namespace Species
{

void PresenceReportCommand::Encode(Neuron::BitWriter& /*_writer*/) const {}

void PresenceReportCommand::Decode(Neuron::BitReader& /*_reader*/) {}

} // namespace Species

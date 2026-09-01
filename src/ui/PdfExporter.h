#pragma once

#include <QString>

namespace domain {
struct ReportData;
}

namespace ui {

// Renderiza un ReportData (armado por domain::ReportEngine) a un PDF
// paginado, con encabezado, logo y tabla. Devuelve false si no se pudo
// escribir el archivo.
bool exportReportToPdf(const domain::ReportData& data, const QString& filePath);

} // namespace ui

#pragma once

#include <QSet>
#include <QString>

namespace domain {

// Arma un codigo/SKU legible a partir de categoria + nombre + variante
// (ej. "UNI-CHAL-XG"), y evita choques contra codigos que ya existen
// agregando un sufijo numerico. Pensado para no depender de un lector
// de codigo de barras: el usuario no tiene que inventar ni capturar
// numeros a mano.
QString generateSku(const QString& categoryName, const QString& productName, const QString& variant,
                     const QSet<QString>& existingSkus);

} // namespace domain

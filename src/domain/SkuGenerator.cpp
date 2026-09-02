#include "domain/SkuGenerator.h"

#include <QStringList>

namespace domain {

namespace {
// Deja solo letras/numeros en mayusculas, sin acentos (ej. "Chaleco
// azul" -> "CHALECOAZUL"), para que el codigo resultante sea estable
// sin importar como se haya capturado el texto original.
QString sanitize(const QString& input) {
    const QString normalized = input.normalized(QString::NormalizationForm_KD);
    QString result;
    for (const QChar& ch : normalized) {
        if (ch.category() == QChar::Mark_NonSpacing) {
            continue;
        }
        if (ch.isLetterOrNumber()) {
            result += ch.toUpper();
        }
    }
    return result;
}
} // namespace

QString generateSku(const QString& categoryName, const QString& productName, const QString& variant,
                     const QSet<QString>& existingSkus) {
    const QString catCode = sanitize(categoryName).left(3);
    const QString nameCode = sanitize(productName).left(4);
    const QString variantCode = sanitize(variant).left(4);

    QStringList parts;
    if (!catCode.isEmpty()) {
        parts << catCode;
    }
    if (!nameCode.isEmpty()) {
        parts << nameCode;
    }
    if (!variantCode.isEmpty()) {
        parts << variantCode;
    }

    QString base = parts.join('-');
    if (base.isEmpty()) {
        base = "SKU";
    }

    QString candidate = base;
    int suffix = 2;
    while (existingSkus.contains(candidate)) {
        candidate = base + "-" + QString::number(suffix);
        ++suffix;
    }
    return candidate;
}

} // namespace domain

#include "data/Models.h"

namespace data {

QString movementTypeToString(MovementType type) {
    switch (type) {
        case MovementType::Entrada: return "entrada";
        case MovementType::Salida: return "salida";
        case MovementType::Ajuste: return "ajuste";
    }
    return "entrada";
}

MovementType movementTypeFromString(const QString& value) {
    if (value == "salida") return MovementType::Salida;
    if (value == "ajuste") return MovementType::Ajuste;
    return MovementType::Entrada;
}

} // namespace data

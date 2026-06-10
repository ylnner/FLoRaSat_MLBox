/*
 * HybridForwardingTable.h
 *
 *  Created on: Jan 6, 2026
 *      Author: ylnner
 */

#ifndef NODES__20_SATELLITE__30_NETWORK_HYBRIDFORWARDINGTABLE_H_
#define NODES__20_SATELLITE__30_NETWORK_HYBRIDFORWARDINGTABLE_H_


#include "ForwardingTable.h"
#include "GroundForwardingTable.h"

class HybridForwardingTable : public cNamedObject {
  private:
    // Instancias de las tablas originales
    ForwardingTable satelliteTable;
    GroundForwardingTable groundTable;

  public:
    HybridForwardingTable(const char *name = nullptr) : cNamedObject(name) {}

    // --- Métodos para Satélites (ForwardingTable) ---
    void setSatelliteRoute(int satId, isldirection::ISLDirection dir) {
        satelliteTable.setRoute(satId, dir);
    }

    // --- Métodos para Tierra (GroundForwardingTable) ---
    void setGroundRoute(int gsId, int firstSat, int lastSat, bool isGS) {
        groundTable.setRoute(gsId, firstSat, lastSat, isGS);
    }

    /**
     * MÉTODO UNIFICADO: La lógica decide a qué tabla consultar
     * basándose en un prefijo o un mapa de tipos de nodos.
     */
    void getNextHopUnified(int destId) {
        // Aquí podrías implementar lógica para saber si destId es satélite o ground
    }

    // Acceso directo si necesitas la tabla completa
    ForwardingTable& getSatelliteTable() { return satelliteTable; }
    GroundForwardingTable& getGroundTable() { return groundTable; }
};


#endif /* NODES__20_SATELLITE__30_NETWORK_HYBRIDFORWARDINGTABLE_H_ */

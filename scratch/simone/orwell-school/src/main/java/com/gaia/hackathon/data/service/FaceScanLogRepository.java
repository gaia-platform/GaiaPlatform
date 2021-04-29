package com.gaia.hackathon.data.service;

import com.gaia.hackathon.data.entity.FaceScanLog;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.JpaSpecificationExecutor;

public interface FaceScanLogRepository extends JpaRepository<FaceScanLog, Void>, JpaSpecificationExecutor<FaceScanLog> {

}
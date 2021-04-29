package com.gaia.hackathon.data.service;

import com.gaia.hackathon.data.entity.Buildings;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.JpaSpecificationExecutor;

public interface BuildingsRepository extends JpaRepository<Buildings, Integer>, JpaSpecificationExecutor<Buildings> {

}
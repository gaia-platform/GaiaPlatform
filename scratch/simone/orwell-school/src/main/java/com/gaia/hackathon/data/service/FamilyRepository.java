package com.gaia.hackathon.data.service;

import com.gaia.hackathon.data.entity.Family;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.JpaSpecificationExecutor;

public interface FamilyRepository extends JpaRepository<Family, Void>, JpaSpecificationExecutor<Family> {

}
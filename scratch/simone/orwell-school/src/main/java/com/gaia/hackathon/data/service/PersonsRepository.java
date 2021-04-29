package com.gaia.hackathon.data.service;

import com.gaia.hackathon.data.entity.Persons;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.JpaSpecificationExecutor;

public interface PersonsRepository extends JpaRepository<Persons, Integer>, JpaSpecificationExecutor<Persons> {

}
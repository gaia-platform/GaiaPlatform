package com.gaia.hackathon.data.service;

import com.gaia.hackathon.data.entity.Strangers;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.JpaSpecificationExecutor;

public interface StrangersRepository extends JpaRepository<Strangers, Void>, JpaSpecificationExecutor<Strangers> {

}
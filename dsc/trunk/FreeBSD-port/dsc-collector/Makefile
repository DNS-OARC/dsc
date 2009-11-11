# New ports collection makefile for:	dnstop
# Date created:		31 October 2002
# Whom:			fenner
#
# $FreeBSD: ports/dns/dnstop/Makefile,v 1.19 2009/02/01 06:42:44 beech Exp $

PORTNAME=	dsc
PKGNAMESUFFIX=  -collector
PORTVERSION=	200910191526
CATEGORIES=	dns
MASTER_SITES=	http://dns.measurement-factory.com/tools/dsc/download/

MAINTAINER=	wessels@dns-oarc.net
COMMENT=	Captures and analyzes DNS traffic

BUILD_DEPENDS=       ${SITE_PERL}/Proc/PID/File.pm:${PORTSDIR}/devel/p5-Proc-PID-File


#GNU_CONFIGURE=	yes
WRKSRC=		${WRKDIR}/${PORTNAME}-${PORTVERSION}/collector

post-install:
	${INSTALL_SCRIPT} ${WRKSRC}/dsc/dsc.sh ${PREFIX}/etc/rc.d/dsc

.include <bsd.port.mk>
